// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "clang_complete.hh"

#include "clang_utils.h"
#include "filesystem.hh"
#include "log.hh"
#include "platform.h"

#include <clang/Frontend/CompilerInstance.h>
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
#include <thread>

namespace ccls {
namespace {

std::string StripFileType(const std::string &path) {
  SmallString<128> Ret;
  sys::path::append(Ret, sys::path::parent_path(path), sys::path::stem(path));
  return Ret.str();
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
  void Flush() {
    if (!last)
      return;
    bool mentions = last->inside_main || last->edits.size();
    if (!mentions)
      for (auto &N : last->notes)
        if (N.inside_main)
          mentions = true;
    if (mentions)
      output.push_back(std::move(*last));
    last.reset();
  }
public:
  std::vector<Diag> Take() {
    return std::move(output);
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
    bool inside_main = SM.isInMainFile(L);
    auto fillDiagBase = [&](DiagBase &d) {
      llvm::SmallString<64> Message;
      Info.FormatDiagnostic(Message);
      d.range =
          FromCharSourceRange(SM, *LangOpts, DiagnosticRange(Info, *LangOpts));
      d.message = Message.str();
      d.inside_main = inside_main;
      d.file = SM.getFilename(Info.getLocation());
      d.level = Level;
      d.category = DiagnosticIDs::getCategoryNumberForDiag(Info.getID());
    };

    auto addFix = [&](bool SyntheticMessage) -> bool {
      if (!inside_main)
        return false;
      for (const FixItHint &FixIt : Info.getFixItHints()) {
        if (!SM.isInMainFile(FixIt.RemoveRange.getBegin()))
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
  for (auto &file : snapshot.files) {
    Bufs.push_back(llvm::MemoryBuffer::getMemBuffer(file.content));
    if (file.filename == session.file.filename) {
      if (auto Preamble = session.GetPreamble()) {
#if LLVM_VERSION_MAJOR >= 7
        Preamble->Preamble.OverridePreamble(*CI, session.FS,
                                            Bufs.back().get());
#else
        Preamble->Preamble.AddImplicitPreamble(*CI, session.FS,
                                               Bufs.back().get());
#endif
      } else {
        CI->getPreprocessorOpts().addRemappedFile(
            CI->getFrontendOpts().Inputs[0].getFile(), Bufs.back().get());
      }
    } else {
      CI->getPreprocessorOpts().addRemappedFile(file.filename,
                                                Bufs.back().get());
    }
  }

  auto Clang = std::make_unique<CompilerInstance>(session.PCH);
  Clang->setInvocation(std::move(CI));
  Clang->setVirtualFileSystem(session.FS);
  Clang->createDiagnostics(&DC, false);
  Clang->setTarget(TargetInfo::CreateTargetInfo(
      Clang->getDiagnostics(), Clang->getInvocation().TargetOpts));
  if (!Clang->hasTarget())
    return nullptr;
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

void CompletionPreloadMain(CompletionManager *manager) {
  while (true) {
    // Fetching the completion request blocks until we have a request.
    auto request = manager->preload_requests_.Dequeue();

    // If we don't get a session then that means we don't care about the file
    // anymore - abandon the request.
    std::shared_ptr<CompletionSession> session = manager->TryGetSession(
        request.path, false /*mark_as_completion*/, false /*create_if_needed*/);
    if (!session)
      continue;

    const auto &args = session->file.args;
    WorkingFiles::Snapshot snapshot = session->wfiles->AsSnapshot(
      {StripFileType(session->file.filename)});

    LOG_S(INFO) << "create completion session for " << session->file.filename;
    if (std::unique_ptr<CompilerInvocation> CI =
            BuildCompilerInvocation(args, session->FS))
      session->BuildPreamble(*CI);
    if (g_config->diagnostics.onSave) {
      lsTextDocumentIdentifier document;
      document.uri = lsDocumentUri::FromPath(request.path);
      manager->diagnostic_request_.PushBack({document}, true);
    }
  }
}

void CompletionMain(CompletionManager *completion_manager) {
  while (true) {
    // Fetching the completion request blocks until we have a request.
    std::unique_ptr<CompletionManager::CompletionRequest> request =
        completion_manager->completion_request_.Dequeue();

    // Drop older requests if we're not buffering.
    while (g_config->completion.dropOldRequests &&
           !completion_manager->completion_request_.IsEmpty()) {
      completion_manager->on_dropped_(request->id);
      request->Consumer.reset();
      request->on_complete(nullptr);
      request = completion_manager->completion_request_.Dequeue();
    }

    std::string path = request->document.uri.GetPath();

    std::shared_ptr<CompletionSession> session =
        completion_manager->TryGetSession(path, true /*mark_as_completion*/,
                                          true /*create_if_needed*/);

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
      completion_manager->working_files_->AsSnapshot({StripFileType(path)});
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
}

void DiagnosticMain(CompletionManager *manager) {
  while (true) {
    // Fetching the completion request blocks until we have a request.
    CompletionManager::DiagnosticRequest request =
        manager->diagnostic_request_.Dequeue();
    std::string path = request.document.uri.GetPath();

    std::shared_ptr<CompletionSession> session = manager->TryGetSession(
        path, true /*mark_as_completion*/, true /*create_if_needed*/);

    std::unique_ptr<CompilerInvocation> CI =
        BuildCompilerInvocation(session->file.args, session->FS);
    if (!CI)
      continue;
    CI->getDiagnosticOpts().IgnoreWarnings = false;
    CI->getLangOpts()->SpellChecking = g_config->diagnostics.spellChecking;
    StoreDiags DC;
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

    std::vector<lsDiagnostic> ls_diags;
    for (auto &d : DC.Take()) {
      if (!d.inside_main)
        continue;
      lsDiagnostic &ls_diag = ls_diags.emplace_back();
      ls_diag.range = lsRange{{d.range.start.line, d.range.start.column},
                              {d.range.end.line, d.range.end.column}};
      ls_diag.message = d.message;
      switch (d.level) {
      case DiagnosticsEngine::Ignored:
        // llvm_unreachable
      case DiagnosticsEngine::Note:
      case DiagnosticsEngine::Remark:
        ls_diag.severity = lsDiagnosticSeverity::Information;
        continue;
      case DiagnosticsEngine::Warning:
        ls_diag.severity = lsDiagnosticSeverity::Warning;
        break;
      case DiagnosticsEngine::Error:
      case DiagnosticsEngine::Fatal:
        ls_diag.severity = lsDiagnosticSeverity::Error;
      }
      ls_diag.code = d.category;
      ls_diag.fixits_ = d.edits;
    }
    manager->on_diagnostic_(path, ls_diags);
  }
}

} // namespace

std::shared_ptr<PreambleData> CompletionSession::GetPreamble() {
  std::lock_guard<std::mutex> lock(mutex);
  return preamble;
}

void CompletionSession::BuildPreamble(CompilerInvocation &CI) {
  std::shared_ptr<PreambleData> OldP = GetPreamble();
  std::string content = wfiles->GetContent(file.filename);
  std::unique_ptr<llvm::MemoryBuffer> Buf =
      llvm::MemoryBuffer::getMemBuffer(content);
  auto Bounds = ComputePreambleBounds(*CI.getLangOpts(), Buf.get(), 0);
  if (OldP && OldP->Preamble.CanReuse(CI, Buf.get(), Bounds, FS.get()))
    return;
  CI.getDiagnosticOpts().IgnoreWarnings = false;
  CI.getFrontendOpts().SkipFunctionBodies = true;
  CI.getLangOpts()->RetainCommentsFromSystemHeaders = true;
  CI.getLangOpts()->CommentOpts.ParseAllComments = true;
#if LLVM_VERSION_MAJOR >= 7
  CI.getPreprocessorOpts().WriteCommentListToPCH = false;
#endif

  StoreDiags DC;
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
      preloaded_sessions_(kMaxPreloadedSessions),
      completion_sessions_(kMaxCompletionSessions),
      PCH(std::make_shared<PCHContainerOperations>()) {
  std::thread([&]() {
    set_thread_name("comp");
    ccls::CompletionMain(this);
  })
      .detach();
  std::thread([&]() {
    set_thread_name("comp-preload");
    ccls::CompletionPreloadMain(this);
  })
      .detach();
  std::thread([&]() {
    set_thread_name("diag");
    ccls::DiagnosticMain(this);
  })
      .detach();
}

void CompletionManager::CodeComplete(
    const lsRequestId &id,
    const lsTextDocumentPositionParams &completion_location,
    const OnComplete &on_complete) {
}

void CompletionManager::DiagnosticsUpdate(
    const lsTextDocumentIdentifier &document) {
  bool has = false;
  diagnostic_request_.Iterate([&](const DiagnosticRequest &request) {
    if (request.document.uri == document.uri)
      has = true;
  });
  if (!has)
    diagnostic_request_.PushBack(DiagnosticRequest{document},
                                 true /*priority*/);
}

void CompletionManager::NotifyView(const std::string &path) {
  // Only reparse the file if we create a new CompletionSession.
  if (EnsureCompletionOrCreatePreloadSession(path))
    preload_requests_.PushBack(PreloadRequest{path}, true);
}

void CompletionManager::NotifySave(const std::string &filename) {
  //
  // On save, always reparse.
  //

  EnsureCompletionOrCreatePreloadSession(filename);
  preload_requests_.PushBack(PreloadRequest{filename}, true);
}

void CompletionManager::NotifyClose(const std::string &filename) {
  //
  // On close, we clear any existing CompletionSession instance.
  //

  std::lock_guard<std::mutex> lock(sessions_lock_);

  // Take and drop. It's okay if we don't actually drop the file, it'll
  // eventually get pushed out of the caches as the user opens other files.
  auto preloaded_ptr = preloaded_sessions_.TryTake(filename);
  LOG_IF_S(INFO, !!preloaded_ptr)
      << "Dropped preloaded-based code completion session for " << filename;
  auto completion_ptr = completion_sessions_.TryTake(filename);
  LOG_IF_S(INFO, !!completion_ptr)
      << "Dropped completion-based code completion session for " << filename;

  // We should never have both a preloaded and completion session.
  assert((preloaded_ptr && completion_ptr) == false);
}

bool CompletionManager::EnsureCompletionOrCreatePreloadSession(
    const std::string &path) {
  std::lock_guard<std::mutex> lock(sessions_lock_);

  // Check for an existing CompletionSession.
  if (preloaded_sessions_.TryGet(path) ||
      completion_sessions_.TryGet(path)) {
    return false;
  }

  // No CompletionSession, create new one.
  auto session = std::make_shared<ccls::CompletionSession>(
      project_->FindCompilationEntryForFile(path), working_files_, PCH);
  preloaded_sessions_.Insert(session->file.filename, session);
  return true;
}

std::shared_ptr<ccls::CompletionSession>
CompletionManager::TryGetSession(const std::string &path,
                                 bool mark_as_completion,
                                 bool create_if_needed) {
  std::lock_guard<std::mutex> lock(sessions_lock_);

  // Try to find a preloaded session.
  std::shared_ptr<ccls::CompletionSession> preloaded =
      preloaded_sessions_.TryGet(path);

  if (preloaded) {
    // If this request is for a completion, we should move it to
    // |completion_sessions|.
    if (mark_as_completion) {
      preloaded_sessions_.TryTake(path);
      completion_sessions_.Insert(path, preloaded);
    }
    return preloaded;
  }

  // Try to find a completion session. If none create one.
  std::shared_ptr<ccls::CompletionSession> session =
      completion_sessions_.TryGet(path);
  if (!session && create_if_needed) {
    session = std::make_shared<ccls::CompletionSession>(
        project_->FindCompilationEntryForFile(path), working_files_, PCH);
    completion_sessions_.Insert(path, session);
  }

  return session;
}

void CompletionManager::FlushSession(const std::string &path) {
  std::lock_guard<std::mutex> lock(sessions_lock_);

  preloaded_sessions_.TryTake(path);
  completion_sessions_.TryTake(path);
}

void CompletionManager::FlushAllSessions() {
  LOG_S(INFO) << "flush all clang complete sessions";
  std::lock_guard<std::mutex> lock(sessions_lock_);

  preloaded_sessions_.Clear();
  completion_sessions_.Clear();
}
