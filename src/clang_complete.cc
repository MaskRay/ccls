/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "clang_complete.hh"

#include "clang_tu.hh"
#include "filesystem.hh"
#include "log.hh"
#include "match.h"
#include "platform.h"

#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Sema/CodeCompleteConsumer.h>
#include <clang/Sema/Sema.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/Support/CrashRecoveryContext.h>
#include <llvm/Support/Threading.h>
using namespace clang;
using namespace llvm;

#include <algorithm>
#include <chrono>
#include <ratio>
#include <thread>
namespace chrono = std::chrono;

namespace ccls {
namespace {

std::string StripFileType(const std::string &path) {
  SmallString<128> Ret;
  sys::path::append(Ret, sys::path::parent_path(path), sys::path::stem(path));
  return sys::path::convert_to_slash(Ret);
}

bool LocationInRange(SourceLocation L, CharSourceRange R,
                     const SourceManager &M) {
  assert(R.isCharRange());
  if (!R.isValid() || M.getFileID(R.getBegin()) != M.getFileID(R.getEnd()) ||
      M.getFileID(R.getBegin()) != M.getFileID(L))
    return false;
  return L != R.getEnd() && M.isPointWithin(L, R.getBegin(), R.getEnd());
}

CharSourceRange DiagnosticRange(const clang::Diagnostic &D, const LangOptions &L) {
  auto &M = D.getSourceManager();
  auto Loc = M.getFileLoc(D.getLocation());
  // Accept the first range that contains the location.
  for (const auto &CR : D.getRanges()) {
    auto R = Lexer::makeFileCharRange(CR, M, L);
    if (LocationInRange(Loc, R, M))
      return R;
  }
  // The range may be given as a fixit hint instead.
  for (const auto &F : D.getFixItHints()) {
    auto R = Lexer::makeFileCharRange(F.RemoveRange, M, L);
    if (LocationInRange(Loc, R, M))
      return R;
  }
  // If no suitable range is found, just use the token at the location.
  auto R = Lexer::makeFileCharRange(CharSourceRange::getTokenRange(Loc), M, L);
  if (!R.isValid()) // Fall back to location only, let the editor deal with it.
    R = CharSourceRange::getCharRange(Loc);
  return R;
}



class StoreDiags : public DiagnosticConsumer {
  const LangOptions *LangOpts;
  std::optional<Diag> last;
  std::vector<Diag> output;
  std::string path;
  std::unordered_map<unsigned, bool> FID2concerned;
  void Flush() {
    if (!last)
      return;
    bool mentions = last->concerned || last->edits.size();
    if (!mentions)
      for (auto &N : last->notes)
        if (N.concerned)
          mentions = true;
    if (mentions)
      output.push_back(std::move(*last));
    last.reset();
  }
public:
  StoreDiags(std::string path) : path(path) {}
  std::vector<Diag> Take() {
    return std::move(output);
  }
  bool IsConcerned(const SourceManager &SM, SourceLocation L) {
    FileID FID = SM.getFileID(L);
    auto it = FID2concerned.try_emplace(FID.getHashValue());
    if (it.second) {
      const FileEntry *FE = SM.getFileEntryForID(FID);
      it.first->second = FE && PathFromFileEntry(*FE) == path;
    }
    return it.first->second;
  }
  void BeginSourceFile(const LangOptions &Opts, const Preprocessor *) override {
    LangOpts = &Opts;
  }
  void EndSourceFile() override {
    Flush();
  }
  void HandleDiagnostic(DiagnosticsEngine::Level Level,
                        const Diagnostic &Info) override {
    DiagnosticConsumer::HandleDiagnostic(Level, Info);
    SourceLocation L = Info.getLocation();
    if (!L.isValid()) return;
    const SourceManager &SM = Info.getSourceManager();
    StringRef Filename = SM.getFilename(Info.getLocation());
    bool concerned = IsConcerned(SM, Info.getLocation());
    auto fillDiagBase = [&](DiagBase &d) {
      llvm::SmallString<64> Message;
      Info.FormatDiagnostic(Message);
      d.range =
          FromCharSourceRange(SM, *LangOpts, DiagnosticRange(Info, *LangOpts));
      d.message = Message.str();
      d.concerned = concerned;
      d.file = Filename;
      d.level = Level;
      d.category = DiagnosticIDs::getCategoryNumberForDiag(Info.getID());
    };

    auto addFix = [&](bool SyntheticMessage) -> bool {
      if (!concerned)
        return false;
      for (const FixItHint &FixIt : Info.getFixItHints()) {
        if (!IsConcerned(SM, FixIt.RemoveRange.getBegin()))
          return false;
        lsTextEdit edit;
        edit.newText = FixIt.CodeToInsert;
        auto r = FromCharSourceRange(SM, *LangOpts, FixIt.RemoveRange);
        edit.range =
            lsRange{{r.start.line, r.start.column}, {r.end.line, r.end.column}};
        last->edits.push_back(std::move(edit));
      }
      return true;
    };

    if (Level == DiagnosticsEngine::Note || Level == DiagnosticsEngine::Remark) {
      if (Info.getFixItHints().size()) {
        addFix(false);
      } else {
        Note &n = last->notes.emplace_back();
        fillDiagBase(n);
      }
    } else {
      Flush();
      last = Diag();
      fillDiagBase(*last);
      if (!Info.getFixItHints().empty())
        addFix(true);
    }
  }
};

std::unique_ptr<CompilerInstance> BuildCompilerInstance(
    CompletionSession &session, std::unique_ptr<CompilerInvocation> CI,
    DiagnosticConsumer &DC, const WorkingFiles::Snapshot &snapshot,
    std::vector<std::unique_ptr<llvm::MemoryBuffer>> &Bufs) {
  std::string main = ResolveIfRelative(
      session.file.directory,
      sys::path::convert_to_slash(CI->getFrontendOpts().Inputs[0].getFile()));
  for (auto &file : snapshot.files) {
    Bufs.push_back(llvm::MemoryBuffer::getMemBuffer(file.content));
    if (file.filename == main)
      if (auto Preamble = session.GetPreamble()) {
#if LLVM_VERSION_MAJOR >= 7
        Preamble->Preamble.OverridePreamble(*CI, session.FS,
                                            Bufs.back().get());
#else
        Preamble->Preamble.AddImplicitPreamble(*CI, session.FS,
                                               Bufs.back().get());
#endif
        continue;
      }
    CI->getPreprocessorOpts().addRemappedFile(file.filename,
      Bufs.back().get());
  }

  auto Clang = std::make_unique<CompilerInstance>(session.PCH);
  Clang->setInvocation(std::move(CI));
  Clang->setVirtualFileSystem(session.FS);
  Clang->createDiagnostics(&DC, false);
  Clang->setTarget(TargetInfo::CreateTargetInfo(
      Clang->getDiagnostics(), Clang->getInvocation().TargetOpts));
  if (!Clang->hasTarget())
    return nullptr;
  // Construct SourceManager with UserFilesAreVolatile: true because otherwise
  // RequiresNullTerminator: true may cause out-of-bounds read when a file is
  // mmap'ed but is saved concurrently.
  Clang->createFileManager();
  Clang->setSourceManager(new SourceManager(Clang->getDiagnostics(),
                                            Clang->getFileManager(), true));
  return Clang;
}

bool Parse(CompilerInstance &Clang) {
  SyntaxOnlyAction Action;
  if (!Action.BeginSourceFile(Clang, Clang.getFrontendOpts().Inputs[0]))
    return false;
  if (!Action.Execute())
    return false;
  Action.EndSourceFile();
  return true;
}

void *CompletionPreloadMain(void *manager_) {
  auto *manager = static_cast<CompletionManager*>(manager_);
  set_thread_name("comp-preload");
  while (true) {
    auto request = manager->preload_requests_.Dequeue();

    bool is_open = false;
    std::shared_ptr<CompletionSession> session =
        manager->TryGetSession(request.path, true, &is_open);
    if (!session)
      continue;

    const auto &args = session->file.args;
    WorkingFiles::Snapshot snapshot =
        session->wfiles->AsSnapshot({StripFileType(session->file.filename)});
    if (std::unique_ptr<CompilerInvocation> CI =
            BuildCompilerInvocation(args, session->FS))
      session->BuildPreamble(*CI, request.path);

    int debounce =
        is_open ? g_config->diagnostics.onOpen : g_config->diagnostics.onSave;
    if (debounce >= 0) {
      lsTextDocumentIdentifier document;
      document.uri = lsDocumentUri::FromPath(request.path);
      manager->DiagnosticsUpdate(request.path, debounce);
    }
  }
  return nullptr;
}

void *CompletionMain(void *manager_) {
  auto *manager = static_cast<CompletionManager *>(manager_);
  set_thread_name("comp");
  while (true) {
    // Fetching the completion request blocks until we have a request.
    std::unique_ptr<CompletionManager::CompletionRequest> request =
        manager->completion_request_.Dequeue();

    // Drop older requests if we're not buffering.
    while (g_config->completion.dropOldRequests &&
           !manager->completion_request_.IsEmpty()) {
      manager->on_dropped_(request->id);
      request->Consumer.reset();
      request->on_complete(nullptr);
      request = manager->completion_request_.Dequeue();
    }

    std::string path = request->document.uri.GetPath();

    std::shared_ptr<CompletionSession> session =
        manager->TryGetSession(path, false);

    std::unique_ptr<CompilerInvocation> CI =
        BuildCompilerInvocation(session->file.args, session->FS);
    if (!CI)
      continue;
    auto &FOpts = CI->getFrontendOpts();
    FOpts.CodeCompleteOpts = request->CCOpts;
    FOpts.CodeCompletionAt.FileName = path;
    FOpts.CodeCompletionAt.Line = request->position.line + 1;
    FOpts.CodeCompletionAt.Column = request->position.character + 1;
    FOpts.SkipFunctionBodies = true;
    CI->getLangOpts()->CommentOpts.ParseAllComments = true;

    DiagnosticConsumer DC;
    WorkingFiles::Snapshot snapshot =
      manager->working_files_->AsSnapshot({StripFileType(path)});
    std::vector<std::unique_ptr<llvm::MemoryBuffer>> Bufs;
    auto Clang = BuildCompilerInstance(*session, std::move(CI), DC, snapshot, Bufs);
    if (!Clang)
      continue;

    Clang->setCodeCompletionConsumer(request->Consumer.release());
    if (!Parse(*Clang))
      continue;
    for (auto &Buf : Bufs)
      Buf.release();

    request->on_complete(&Clang->getCodeCompletionConsumer());
  }
  return nullptr;
}

llvm::StringRef diagLeveltoString(DiagnosticsEngine::Level Lvl) {
  switch (Lvl) {
  case DiagnosticsEngine::Ignored:
    return "ignored";
  case DiagnosticsEngine::Note:
    return "note";
  case DiagnosticsEngine::Remark:
    return "remark";
  case DiagnosticsEngine::Warning:
    return "warning";
  case DiagnosticsEngine::Error:
    return "error";
  case DiagnosticsEngine::Fatal:
    return "fatal error";
  }
}

void printDiag(llvm::raw_string_ostream &OS, const DiagBase &d) {
  if (d.concerned)
    OS << llvm::sys::path::filename(d.file);
  else
    OS << d.file;
  auto pos = d.range.start;
  OS << ":" << (pos.line + 1) << ":" << (pos.column + 1) << ":"
     << (d.concerned ? " " : "\n");
  OS << diagLeveltoString(d.level) << ": " << d.message;
}

void *DiagnosticMain(void *manager_) {
  auto *manager = static_cast<CompletionManager*>(manager_);
  set_thread_name("diag");
  while (true) {
    CompletionManager::DiagnosticRequest request =
        manager->diagnostic_request_.Dequeue();
    const std::string &path = request.path;
    int64_t wait = request.wait_until -
                   chrono::duration_cast<chrono::milliseconds>(
                       chrono::high_resolution_clock::now().time_since_epoch())
                       .count();
    if (wait > 0)
      std::this_thread::sleep_for(chrono::duration<int64_t, std::milli>(
          std::min(wait, request.debounce)));

    std::shared_ptr<CompletionSession> session =
        manager->TryGetSession(path, false);

    std::unique_ptr<CompilerInvocation> CI =
        BuildCompilerInvocation(session->file.args, session->FS);
    if (!CI)
      continue;
    CI->getDiagnosticOpts().IgnoreWarnings = false;
    CI->getLangOpts()->SpellChecking = g_config->diagnostics.spellChecking;
    StoreDiags DC(path);
    WorkingFiles::Snapshot snapshot =
        manager->working_files_->AsSnapshot({StripFileType(path)});
    std::vector<std::unique_ptr<llvm::MemoryBuffer>> Bufs;
    auto Clang = BuildCompilerInstance(*session, std::move(CI), DC, snapshot, Bufs);
    if (!Clang)
      continue;
    if (!Parse(*Clang))
      continue;
    for (auto &Buf : Bufs)
      Buf.release();

    auto Fill = [](const DiagBase &d, lsDiagnostic &ret) {
      ret.range = lsRange{{d.range.start.line, d.range.start.column},
                          {d.range.end.line, d.range.end.column}};
      switch (d.level) {
      case DiagnosticsEngine::Ignored:
        // llvm_unreachable
      case DiagnosticsEngine::Remark:
        ret.severity = lsDiagnosticSeverity::Hint;
        break;
      case DiagnosticsEngine::Note:
        ret.severity = lsDiagnosticSeverity::Information;
        break;
      case DiagnosticsEngine::Warning:
        ret.severity = lsDiagnosticSeverity::Warning;
        break;
      case DiagnosticsEngine::Error:
      case DiagnosticsEngine::Fatal:
        ret.severity = lsDiagnosticSeverity::Error;
        break;
      }
      ret.code = d.category;
      return ret;
    };

    std::vector<Diag> diags = DC.Take();
    if (std::shared_ptr<PreambleData> preamble = session->GetPreamble())
      diags.insert(diags.end(), preamble->diags.begin(), preamble->diags.end());
    std::vector<lsDiagnostic> ls_diags;
    for (auto &d : diags) {
      if (!d.concerned)
        continue;
      std::string buf;
      llvm::raw_string_ostream OS(buf);
      lsDiagnostic &ls_diag = ls_diags.emplace_back();
      Fill(d, ls_diag);
      ls_diag.fixits_ = d.edits;
      OS << d.message;
      for (auto &n : d.notes) {
        OS << "\n\n";
        printDiag(OS, n);
      }
      OS.flush();
      ls_diag.message = std::move(buf);
      for (auto &n : d.notes) {
        if (!n.concerned)
          continue;
        lsDiagnostic &ls_diag1 = ls_diags.emplace_back();
        Fill(n, ls_diag1);
        OS << n.message << "\n\n";
        printDiag(OS, d);
        OS.flush();
        ls_diag1.message = std::move(buf);
      }
    }

    {
      std::lock_guard lock(session->wfiles->files_mutex);
      if (WorkingFile *wfile = session->wfiles->GetFileByFilenameNoLock(path))
        wfile->diagnostics_ = ls_diags;
    }
    manager->on_diagnostic_(path, ls_diags);
  }
  return nullptr;
}

} // namespace

std::shared_ptr<PreambleData> CompletionSession::GetPreamble() {
  std::lock_guard<std::mutex> lock(mutex);
  return preamble;
}

void CompletionSession::BuildPreamble(CompilerInvocation &CI,
                                      const std::string &main) {
  std::shared_ptr<PreambleData> OldP = GetPreamble();
  std::string content = wfiles->GetContent(main);
  std::unique_ptr<llvm::MemoryBuffer> Buf =
      llvm::MemoryBuffer::getMemBuffer(content);
  auto Bounds = ComputePreambleBounds(*CI.getLangOpts(), Buf.get(), 0);
  if (OldP && OldP->Preamble.CanReuse(CI, Buf.get(), Bounds, FS.get()))
    return;
  CI.getDiagnosticOpts().IgnoreWarnings = false;
  CI.getFrontendOpts().SkipFunctionBodies = true;
  CI.getLangOpts()->CommentOpts.ParseAllComments = true;
#if LLVM_VERSION_MAJOR >= 7
  CI.getPreprocessorOpts().WriteCommentListToPCH = false;
#endif

  StoreDiags DC(main);
  IntrusiveRefCntPtr<DiagnosticsEngine> DE =
      CompilerInstance::createDiagnostics(&CI.getDiagnosticOpts(), &DC, false);
  PreambleCallbacks PP;
  if (auto NewPreamble = PrecompiledPreamble::Build(CI, Buf.get(), Bounds,
      *DE, FS, PCH, true, PP)) {
    std::lock_guard<std::mutex> lock(mutex);
    preamble =
        std::make_shared<PreambleData>(std::move(*NewPreamble), DC.Take());
  }
}

} // namespace ccls

CompletionManager::CompletionManager(Project *project,
                                     WorkingFiles *working_files,
                                     OnDiagnostic on_diagnostic,
                                     OnDropped on_dropped)
    : project_(project), working_files_(working_files),
      on_diagnostic_(on_diagnostic), on_dropped_(on_dropped),
      preloads(kMaxPreloadedSessions),
      sessions(kMaxCompletionSessions),
      PCH(std::make_shared<PCHContainerOperations>()) {
  SpawnThread(ccls::CompletionMain, this);
  SpawnThread(ccls::CompletionPreloadMain, this);
  SpawnThread(ccls::DiagnosticMain, this);
}

void CompletionManager::DiagnosticsUpdate(const std::string &path,
                                          int debounce) {
  static GroupMatch match(g_config->diagnostics.whitelist,
                          g_config->diagnostics.blacklist);
  if (!match.IsMatch(path))
    return;
  int64_t now = chrono::duration_cast<chrono::milliseconds>(
    chrono::high_resolution_clock::now().time_since_epoch())
    .count();
  bool flag = false;
  {
    std::lock_guard lock(diag_mutex);
    int64_t &next = next_diag[path];
    auto &d = g_config->diagnostics;
    if (next <= now ||
      now - next > std::max(d.onChange, std::max(d.onChange, d.onSave))) {
      next = now + debounce;
      flag = true;
    }
  }
  if (flag)
    diagnostic_request_.PushBack({path, now + debounce, debounce}, false);
}

void CompletionManager::NotifyView(const std::string &path) {
  // Only reparse the file if we create a new CompletionSession.
  if (EnsureCompletionOrCreatePreloadSession(path))
    preload_requests_.PushBack(PreloadRequest{path}, true);
}

void CompletionManager::NotifySave(const std::string &filename) {
  EnsureCompletionOrCreatePreloadSession(filename);
  preload_requests_.PushBack(PreloadRequest{filename}, true);
}

void CompletionManager::OnClose(const std::string &filename) {
  std::lock_guard<std::mutex> lock(sessions_lock_);
  preloads.TryTake(filename);
  sessions.TryTake(filename);
}

bool CompletionManager::EnsureCompletionOrCreatePreloadSession(
    const std::string &path) {
  std::lock_guard<std::mutex> lock(sessions_lock_);
  if (preloads.TryGet(path) || sessions.TryGet(path))
    return false;

  // No CompletionSession, create new one.
  auto session = std::make_shared<ccls::CompletionSession>(
      project_->FindEntry(path, false), working_files_, PCH);
  if (session->file.filename != path) {
    session->inferred = true;
    session->file.filename = path;
  }
  preloads.Insert(path, session);
  LOG_S(INFO) << "create preload session for " << path;
  return true;
}

std::shared_ptr<ccls::CompletionSession>
CompletionManager::TryGetSession(const std::string &path, bool preload,
                                 bool *is_open) {
  std::lock_guard<std::mutex> lock(sessions_lock_);
  std::shared_ptr<ccls::CompletionSession> session = preloads.TryGet(path);

  if (session) {
    if (!preload) {
      preloads.TryTake(path);
      sessions.Insert(path, session);
      if (is_open)
        *is_open = true;
    }
    return session;
  }

  session = sessions.TryGet(path);
  if (!session && !preload) {
    session = std::make_shared<ccls::CompletionSession>(
        project_->FindEntry(path, false), working_files_, PCH);
    sessions.Insert(path, session);
    LOG_S(INFO) << "create session for " << path;
    if (is_open)
      *is_open = true;
  }

  return session;
}

void CompletionManager::FlushAllSessions() {
  LOG_S(INFO) << "flush all clang complete sessions";
  std::lock_guard<std::mutex> lock(sessions_lock_);

  preloads.Clear();
  sessions.Clear();
}
