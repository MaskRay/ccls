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

#include "sema_manager.hh"

#include "clang_tu.hh"
#include "filesystem.hh"
#include "log.hh"
#include "pipeline.hh"
#include "platform.hh"

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

#if LLVM_VERSION_MAJOR < 8
namespace clang::vfs {
struct ProxyFileSystem : FileSystem {
  explicit ProxyFileSystem(IntrusiveRefCntPtr<FileSystem> FS)
      : FS(std::move(FS)) {}
  llvm::ErrorOr<Status> status(const Twine &Path) override {
    return FS->status(Path);
  }
  llvm::ErrorOr<std::unique_ptr<File>>
  openFileForRead(const Twine &Path) override {
    return FS->openFileForRead(Path);
  }
  directory_iterator dir_begin(const Twine &Dir, std::error_code &EC) override {
    return FS->dir_begin(Dir, EC);
  }
  llvm::ErrorOr<std::string> getCurrentWorkingDirectory() const override {
    return FS->getCurrentWorkingDirectory();
  }
  std::error_code setCurrentWorkingDirectory(const Twine &Path) override {
    return FS->setCurrentWorkingDirectory(Path);
  }
#if LLVM_VERSION_MAJOR == 7
  std::error_code getRealPath(const Twine &Path,
                              SmallVectorImpl<char> &Output) const override {
    return FS->getRealPath(Path, Output);
  }
#endif
  FileSystem &getUnderlyingFS() { return *FS; }
  IntrusiveRefCntPtr<FileSystem> FS;
};
}
#endif

namespace ccls {

TextEdit ToTextEdit(const clang::SourceManager &SM, const clang::LangOptions &L,
                    const clang::FixItHint &FixIt) {
  TextEdit edit;
  edit.newText = FixIt.CodeToInsert;
  auto r = FromCharSourceRange(SM, L, FixIt.RemoveRange);
  edit.range =
      lsRange{{r.start.line, r.start.column}, {r.end.line, r.end.column}};
  return edit;
}

using IncludeStructure = std::vector<std::pair<std::string, int64_t>>;

struct PreambleStatCache {
  llvm::StringMap<ErrorOr<llvm::vfs::Status>> Cache;

  void Update(Twine Path, ErrorOr<llvm::vfs::Status> S) {
    Cache.try_emplace(Path.str(), std::move(S));
  }

  IntrusiveRefCntPtr<llvm::vfs::FileSystem>
  Producer(IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS) {
    struct VFS : llvm::vfs::ProxyFileSystem {
      PreambleStatCache &Cache;

      VFS(IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS,
          PreambleStatCache &Cache)
          : ProxyFileSystem(std::move(FS)), Cache(Cache) {}
      llvm::ErrorOr<std::unique_ptr<llvm::vfs::File>>
      openFileForRead(const Twine &Path) override {
        auto File = getUnderlyingFS().openFileForRead(Path);
        if (!File || !*File)
          return File;
        Cache.Update(Path, File->get()->status());
        return File;
      }
      llvm::ErrorOr<llvm::vfs::Status> status(const Twine &Path) override {
        auto S = getUnderlyingFS().status(Path);
        Cache.Update(Path, S);
        return S;
      }
    };
    return new VFS(std::move(FS), *this);
  }

  IntrusiveRefCntPtr<llvm::vfs::FileSystem>
  Consumer(IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS) {
    struct VFS : llvm::vfs::ProxyFileSystem {
      const PreambleStatCache &Cache;
      VFS(IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS,
          const PreambleStatCache &Cache)
          : ProxyFileSystem(std::move(FS)), Cache(Cache) {}
      llvm::ErrorOr<llvm::vfs::Status> status(const Twine &Path) override {
        auto I = Cache.Cache.find(Path.str());
        if (I != Cache.Cache.end())
          return I->getValue();
        return getUnderlyingFS().status(Path);
      }
    };
    return new VFS(std::move(FS), *this);
  }
};

struct PreambleData {
  PreambleData(clang::PrecompiledPreamble P, IncludeStructure includes,
               std::vector<Diag> diags,
               std::unique_ptr<PreambleStatCache> stat_cache)
      : Preamble(std::move(P)), includes(std::move(includes)),
        diags(std::move(diags)), stat_cache(std::move(stat_cache)) {}
  clang::PrecompiledPreamble Preamble;
  IncludeStructure includes;
  std::vector<Diag> diags;
  std::unique_ptr<PreambleStatCache> stat_cache;
};

namespace {
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

class StoreInclude : public PPCallbacks {
  const SourceManager &SM;
  IncludeStructure &out;
  DenseSet<const FileEntry *> Seen;

public:
  StoreInclude(const SourceManager &SM, IncludeStructure &out)
      : SM(SM), out(out) {}
  void InclusionDirective(SourceLocation HashLoc, const Token &IncludeTok,
                          StringRef FileName, bool IsAngled,
                          CharSourceRange FilenameRange, const FileEntry *File,
                          StringRef SearchPath, StringRef RelativePath,
                          const clang::Module *Imported
#if LLVM_VERSION_MAJOR >= 7
                          , SrcMgr::CharacteristicKind FileKind
#endif
                          ) override {
    (void)SM;
    if (File && Seen.insert(File).second)
      out.emplace_back(PathFromFileEntry(*File), File->getModificationTime());
  }
};

class CclsPreambleCallbacks : public PreambleCallbacks {
public:
  void BeforeExecute(CompilerInstance &CI) override {
    SM = &CI.getSourceManager();
  }
  std::unique_ptr<PPCallbacks> createPPCallbacks() override {
    return std::make_unique<StoreInclude>(*SM, includes);
  }
  SourceManager *SM = nullptr;
  IncludeStructure includes;
};

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
                        const clang::Diagnostic &Info) override {
    DiagnosticConsumer::HandleDiagnostic(Level, Info);
    SourceLocation L = Info.getLocation();
    if (!L.isValid()) return;
    const SourceManager &SM = Info.getSourceManager();
    StringRef Filename = SM.getFilename(Info.getLocation());
    bool concerned = SM.isWrittenInMainFile(L);
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
        last->edits.push_back(ToTextEdit(SM, *LangOpts, FixIt));
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
    Session &session, std::unique_ptr<CompilerInvocation> CI,
    IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS, DiagnosticConsumer &DC,
    const PreambleData *preamble, const std::string &main,
    std::unique_ptr<llvm::MemoryBuffer> &Buf) {
  if (preamble) {
#if LLVM_VERSION_MAJOR >= 7
    preamble->Preamble.OverridePreamble(*CI, FS, Buf.get());
#else
    preamble->Preamble.AddImplicitPreamble(*CI, FS, Buf.get());
#endif
  } else {
    CI->getPreprocessorOpts().addRemappedFile(main, Buf.get());
  }

  auto Clang = std::make_unique<CompilerInstance>(session.PCH);
  Clang->setInvocation(std::move(CI));
  Clang->setVirtualFileSystem(FS);
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
  auto &IS = Clang->getFrontendOpts().Inputs;
  if (IS.size()) {
    assert(IS[0].isFile());
    IS[0] = FrontendInputFile(main, IS[0].getKind(), IS[0].isSystem());
  }
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

void BuildPreamble(Session &session, CompilerInvocation &CI,
                   IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS,
                   const SemaManager::PreambleTask &task,
                   std::unique_ptr<PreambleStatCache> stat_cache) {
  std::shared_ptr<PreambleData> OldP = session.GetPreamble();
  std::string content = session.wfiles->GetContent(task.path);
  std::unique_ptr<llvm::MemoryBuffer> Buf =
      llvm::MemoryBuffer::getMemBuffer(content);
  auto Bounds = ComputePreambleBounds(*CI.getLangOpts(), Buf.get(), 0);
  if (!task.from_diag && OldP &&
      OldP->Preamble.CanReuse(CI, Buf.get(), Bounds, FS.get()))
    return;
  // -Werror makes warnings issued as errors, which stops parsing
  // prematurely because of -ferror-limit=. This also works around the issue
  // of -Werror + -Wunused-parameter in interaction with SkipFunctionBodies.
  auto &Ws = CI.getDiagnosticOpts().Warnings;
  Ws.erase(std::remove(Ws.begin(), Ws.end(), "error"), Ws.end());
  CI.getDiagnosticOpts().IgnoreWarnings = false;
  CI.getFrontendOpts().SkipFunctionBodies = true;
  CI.getLangOpts()->CommentOpts.ParseAllComments = g_config->index.comments > 1;

  StoreDiags DC(task.path);
  IntrusiveRefCntPtr<DiagnosticsEngine> DE =
      CompilerInstance::createDiagnostics(&CI.getDiagnosticOpts(), &DC, false);
  if (OldP) {
    std::lock_guard lock(session.wfiles->mutex);
    for (auto &include : OldP->includes)
      if (WorkingFile *wf = session.wfiles->GetFileUnlocked(include.first))
        CI.getPreprocessorOpts().addRemappedFile(
            include.first,
            llvm::MemoryBuffer::getMemBufferCopy(wf->buffer_content).release());
  }

  CclsPreambleCallbacks PC;
  if (auto NewPreamble = PrecompiledPreamble::Build(
          CI, Buf.get(), Bounds, *DE, FS, session.PCH, true, PC)) {
    assert(!CI.getPreprocessorOpts().RetainRemappedFileBuffers);
    if (OldP) {
      auto &old_includes = OldP->includes;
      auto it = old_includes.begin();
      std::sort(PC.includes.begin(), PC.includes.end());
      for (auto &include : PC.includes)
        if (include.second == 0) {
          while (it != old_includes.end() && it->first < include.first)
            ++it;
          if (it == old_includes.end())
            break;
          include.second = it->second;
        }
    }

    std::lock_guard lock(session.mutex);
    session.preamble = std::make_shared<PreambleData>(
        std::move(*NewPreamble), std::move(PC.includes), DC.Take(),
        std::move(stat_cache));
  }
}

void *PreambleMain(void *manager_) {
  auto *manager = static_cast<SemaManager *>(manager_);
  set_thread_name("preamble");
  while (true) {
    SemaManager::PreambleTask task = manager->preamble_tasks.Dequeue();
    if (pipeline::quit.load(std::memory_order_relaxed))
      break;

    bool created = false;
    std::shared_ptr<Session> session =
        manager->EnsureSession(task.path, &created);

    auto stat_cache = std::make_unique<PreambleStatCache>();
    IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS =
        stat_cache->Producer(session->FS);
    if (std::unique_ptr<CompilerInvocation> CI =
            BuildCompilerInvocation(session->file.args, FS))
      BuildPreamble(*session, *CI, FS, task, std::move(stat_cache));

    if (task.from_diag) {
      manager->ScheduleDiag(task.path, 0);
    } else {
      int debounce =
          created ? g_config->diagnostics.onOpen : g_config->diagnostics.onSave;
      if (debounce >= 0)
        manager->ScheduleDiag(task.path, debounce);
    }
  }
  pipeline::ThreadLeave();
  return nullptr;
}

void *CompletionMain(void *manager_) {
  auto *manager = static_cast<SemaManager *>(manager_);
  set_thread_name("comp");
  while (true) {
    std::unique_ptr<SemaManager::CompTask> task = manager->comp_tasks.Dequeue();
    if (pipeline::quit.load(std::memory_order_relaxed))
      break;

    // Drop older requests if we're not buffering.
    while (g_config->completion.dropOldRequests &&
           !manager->comp_tasks.IsEmpty()) {
      manager->on_dropped_(task->id);
      task->Consumer.reset();
      task->on_complete(nullptr);
      task = manager->comp_tasks.Dequeue();
      if (pipeline::quit.load(std::memory_order_relaxed))
        break;
    }

    std::shared_ptr<Session> session = manager->EnsureSession(task->path);
    std::shared_ptr<PreambleData> preamble = session->GetPreamble();
    IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS =
        preamble ? preamble->stat_cache->Consumer(session->FS) : session->FS;
    std::unique_ptr<CompilerInvocation> CI =
        BuildCompilerInvocation(session->file.args, FS);
    if (!CI)
      continue;
    auto &FOpts = CI->getFrontendOpts();
    FOpts.CodeCompleteOpts = task->CCOpts;
    FOpts.CodeCompletionAt.FileName = task->path;
    FOpts.CodeCompletionAt.Line = task->position.line + 1;
    FOpts.CodeCompletionAt.Column = task->position.character + 1;
    FOpts.SkipFunctionBodies = true;
    CI->getLangOpts()->CommentOpts.ParseAllComments = true;

    DiagnosticConsumer DC;
    std::string content = manager->wfiles->GetContent(task->path);
    auto Buf = llvm::MemoryBuffer::getMemBuffer(content);
    bool in_preamble =
        GetOffsetForPosition({task->position.line, task->position.character},
                             content) <
        ComputePreambleBounds(*CI->getLangOpts(), Buf.get(), 0).Size;
    if (in_preamble)
      preamble.reset();
    auto Clang = BuildCompilerInstance(*session, std::move(CI), FS, DC,
                                       preamble.get(), task->path, Buf);
    if (!Clang)
      continue;

    Clang->getPreprocessorOpts().SingleFileParseMode = in_preamble;
    Clang->setCodeCompletionConsumer(task->Consumer.release());
    if (!Parse(*Clang))
      continue;
    Buf.release();

    task->on_complete(&Clang->getCodeCompletionConsumer());
  }
  pipeline::ThreadLeave();
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
  auto *manager = static_cast<SemaManager *>(manager_);
  set_thread_name("diag");
  while (true) {
    SemaManager::DiagTask task = manager->diag_tasks.Dequeue();
    if (pipeline::quit.load(std::memory_order_relaxed))
      break;
    int64_t wait = task.wait_until -
                   chrono::duration_cast<chrono::milliseconds>(
                       chrono::high_resolution_clock::now().time_since_epoch())
                       .count();
    if (wait > 0)
      std::this_thread::sleep_for(
          chrono::duration<int64_t, std::milli>(std::min(wait, task.debounce)));

    std::shared_ptr<Session> session = manager->EnsureSession(task.path);
    std::shared_ptr<PreambleData> preamble = session->GetPreamble();
    IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS =
        preamble ? preamble->stat_cache->Consumer(session->FS) : session->FS;
    if (preamble) {
      bool rebuild = false;
      {
        std::lock_guard lock(manager->wfiles->mutex);
        for (auto &include : preamble->includes)
          if (WorkingFile *wf = manager->wfiles->GetFileUnlocked(include.first);
              wf && include.second < wf->timestamp) {
            include.second = wf->timestamp;
            rebuild = true;
          }
      }
      if (rebuild) {
        manager->preamble_tasks.PushBack({task.path, true}, true);
        continue;
      }
    }

    std::unique_ptr<CompilerInvocation> CI =
        BuildCompilerInvocation(session->file.args, FS);
    if (!CI)
      continue;
    // If main file is a header, add -Wno-unused-function
    if (lookupExtension(session->file.filename).second)
      CI->getDiagnosticOpts().Warnings.push_back("no-unused-function");
    CI->getDiagnosticOpts().IgnoreWarnings = false;
    CI->getLangOpts()->SpellChecking = g_config->diagnostics.spellChecking;
    StoreDiags DC(task.path);
    std::string content = manager->wfiles->GetContent(task.path);
    auto Buf = llvm::MemoryBuffer::getMemBuffer(content);
    auto Clang = BuildCompilerInstance(*session, std::move(CI), FS, DC,
                                       preamble.get(), task.path, Buf);
    if (!Clang)
      continue;
    if (!Parse(*Clang))
      continue;
    Buf.release();

    auto Fill = [](const DiagBase &d, Diagnostic &ret) {
      ret.range = lsRange{{d.range.start.line, d.range.start.column},
                          {d.range.end.line, d.range.end.column}};
      switch (d.level) {
      case DiagnosticsEngine::Ignored:
        // llvm_unreachable
      case DiagnosticsEngine::Remark:
        ret.severity = 4;
        break;
      case DiagnosticsEngine::Note:
        ret.severity = 3;
        break;
      case DiagnosticsEngine::Warning:
        ret.severity = 2;
        break;
      case DiagnosticsEngine::Error:
      case DiagnosticsEngine::Fatal:
        ret.severity = 1;
        break;
      }
      ret.code = d.category;
      return ret;
    };

    std::vector<Diag> diags = DC.Take();
    if (std::shared_ptr<PreambleData> preamble = session->GetPreamble())
      diags.insert(diags.end(), preamble->diags.begin(), preamble->diags.end());
    std::vector<Diagnostic> ls_diags;
    for (auto &d : diags) {
      if (!d.concerned)
        continue;
      std::string buf;
      llvm::raw_string_ostream OS(buf);
      Diagnostic &ls_diag = ls_diags.emplace_back();
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
        Diagnostic &ls_diag1 = ls_diags.emplace_back();
        Fill(n, ls_diag1);
        OS << n.message << "\n\n";
        printDiag(OS, d);
        OS.flush();
        ls_diag1.message = std::move(buf);
      }
    }

    {
      std::lock_guard lock(manager->wfiles->mutex);
      if (WorkingFile *wf = manager->wfiles->GetFileUnlocked(task.path))
        wf->diagnostics = ls_diags;
    }
    manager->on_diagnostic_(task.path, ls_diags);
  }
  pipeline::ThreadLeave();
  return nullptr;
}

} // namespace

std::shared_ptr<PreambleData> Session::GetPreamble() {
  std::lock_guard<std::mutex> lock(mutex);
  return preamble;
}

SemaManager::SemaManager(Project *project, WorkingFiles *wfiles,
                         OnDiagnostic on_diagnostic, OnDropped on_dropped)
    : project_(project), wfiles(wfiles), on_diagnostic_(on_diagnostic),
      on_dropped_(on_dropped), PCH(std::make_shared<PCHContainerOperations>()) {
  SpawnThread(ccls::PreambleMain, this);
  SpawnThread(ccls::CompletionMain, this);
  SpawnThread(ccls::DiagnosticMain, this);
}

void SemaManager::ScheduleDiag(const std::string &path, int debounce) {
  static GroupMatch match(g_config->diagnostics.whitelist,
                          g_config->diagnostics.blacklist);
  if (!match.Matches(path))
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
    diag_tasks.PushBack({path, now + debounce, debounce}, false);
}

void SemaManager::OnView(const std::string &path) {
  std::lock_guard lock(mutex);
  if (!sessions.Get(path))
    preamble_tasks.PushBack(PreambleTask{path}, true);
}

void SemaManager::OnSave(const std::string &path) {
  preamble_tasks.PushBack(PreambleTask{path}, true);
}

void SemaManager::OnClose(const std::string &path) {
  std::lock_guard lock(mutex);
  sessions.Take(path);
}

std::shared_ptr<ccls::Session>
SemaManager::EnsureSession(const std::string &path, bool *created) {
  std::lock_guard lock(mutex);
  std::shared_ptr<ccls::Session> session = sessions.Get(path);
  if (!session) {
    session = std::make_shared<ccls::Session>(
        project_->FindEntry(path, true), wfiles, PCH);
    std::string line;
    if (LOG_V_ENABLED(1)) {
      line = "\n ";
      for (auto &arg : session->file.args)
        (line += ' ') += arg;
    }
    LOG_S(INFO) << "create session for " << path << line;
    sessions.Insert(path, session);
    if (created)
      *created = true;
  }
  return session;
}

void SemaManager::Clear() {
  LOG_S(INFO) << "clear all sessions";
  std::lock_guard lock(mutex);
  sessions.Clear();
}

void SemaManager::Quit() {
  comp_tasks.PushBack(nullptr);
  diag_tasks.PushBack({});
  preamble_tasks.PushBack({});
}
} // namespace ccls
