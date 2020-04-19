// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "sema_manager.hh"

#include "clang_tu.hh"
#include "filesystem.hh"
#include "log.hh"
#include "pipeline.hh"
#include "platform.hh"

#include <clang/Basic/TargetInfo.h>
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
} // namespace clang::vfs
#endif

namespace ccls {

TextEdit toTextEdit(const clang::SourceManager &sm, const clang::LangOptions &l,
                    const clang::FixItHint &fixIt) {
  TextEdit edit;
  edit.newText = fixIt.CodeToInsert;
  auto r = fromCharSourceRange(sm, l, fixIt.RemoveRange);
  edit.range =
      lsRange{{r.start.line, r.start.column}, {r.end.line, r.end.column}};
  return edit;
}

using IncludeStructure = std::vector<std::pair<std::string, int64_t>>;

struct PreambleStatCache {
  llvm::StringMap<ErrorOr<llvm::vfs::Status>> cache;

  void update(Twine path, ErrorOr<llvm::vfs::Status> s) {
    cache.try_emplace(path.str(), std::move(s));
  }

  IntrusiveRefCntPtr<llvm::vfs::FileSystem>
  producer(IntrusiveRefCntPtr<llvm::vfs::FileSystem> fs) {
    struct VFS : llvm::vfs::ProxyFileSystem {
      PreambleStatCache &cache;

      VFS(IntrusiveRefCntPtr<llvm::vfs::FileSystem> fs,
          PreambleStatCache &cache)
          : ProxyFileSystem(std::move(fs)), cache(cache) {}
      llvm::ErrorOr<std::unique_ptr<llvm::vfs::File>>
      openFileForRead(const Twine &path) override {
        auto file = getUnderlyingFS().openFileForRead(path);
        if (!file || !*file)
          return file;
        cache.update(path, file->get()->status());
        return file;
      }
      llvm::ErrorOr<llvm::vfs::Status> status(const Twine &path) override {
        auto s = getUnderlyingFS().status(path);
        cache.update(path, s);
        return s;
      }
    };
    return new VFS(std::move(fs), *this);
  }

  IntrusiveRefCntPtr<llvm::vfs::FileSystem>
  consumer(IntrusiveRefCntPtr<llvm::vfs::FileSystem> fs) {
    struct VFS : llvm::vfs::ProxyFileSystem {
      const PreambleStatCache &cache;
      VFS(IntrusiveRefCntPtr<llvm::vfs::FileSystem> fs,
          const PreambleStatCache &cache)
          : ProxyFileSystem(std::move(fs)), cache(cache) {}
      llvm::ErrorOr<llvm::vfs::Status> status(const Twine &path) override {
        auto i = cache.cache.find(path.str());
        if (i != cache.cache.end())
          return i->getValue();
        return getUnderlyingFS().status(path);
      }
    };
    return new VFS(std::move(fs), *this);
  }
};

struct PreambleData {
  PreambleData(clang::PrecompiledPreamble p, IncludeStructure includes,
               std::vector<Diag> diags,
               std::unique_ptr<PreambleStatCache> stat_cache)
      : preamble(std::move(p)), includes(std::move(includes)),
        diags(std::move(diags)), stat_cache(std::move(stat_cache)) {}
  clang::PrecompiledPreamble preamble;
  IncludeStructure includes;
  std::vector<Diag> diags;
  std::unique_ptr<PreambleStatCache> stat_cache;
};

namespace {
bool locationInRange(SourceLocation l, CharSourceRange r,
                     const SourceManager &m) {
  assert(r.isCharRange());
  if (!r.isValid() || m.getFileID(r.getBegin()) != m.getFileID(r.getEnd()) ||
      m.getFileID(r.getBegin()) != m.getFileID(l))
    return false;
  return l != r.getEnd() && m.isPointWithin(l, r.getBegin(), r.getEnd());
}

CharSourceRange diagnosticRange(const clang::Diagnostic &d,
                                const LangOptions &l) {
  auto &m = d.getSourceManager();
  auto loc = m.getFileLoc(d.getLocation());
  // Accept the first range that contains the location.
  for (const auto &cr : d.getRanges()) {
    auto r = Lexer::makeFileCharRange(cr, m, l);
    if (locationInRange(loc, r, m))
      return r;
  }
  // The range may be given as a fixit hint instead.
  for (const auto &f : d.getFixItHints()) {
    auto r = Lexer::makeFileCharRange(f.RemoveRange, m, l);
    if (locationInRange(loc, r, m))
      return r;
  }
  // If no suitable range is found, just use the token at the location.
  auto r = Lexer::makeFileCharRange(CharSourceRange::getTokenRange(loc), m, l);
  if (!r.isValid()) // Fall back to location only, let the editor deal with it.
    r = CharSourceRange::getCharRange(loc);
  return r;
}

class StoreInclude : public PPCallbacks {
  const SourceManager &sm;
  IncludeStructure &out;
  DenseSet<const FileEntry *> seen;

public:
  StoreInclude(const SourceManager &sm, IncludeStructure &out)
      : sm(sm), out(out) {}
  void InclusionDirective(SourceLocation hashLoc, const Token &includeTok,
                          StringRef fileName, bool isAngled,
                          CharSourceRange filenameRange, const FileEntry *file,
                          StringRef searchPath, StringRef relativePath,
                          const clang::Module *imported,
                          SrcMgr::CharacteristicKind fileKind) override {
    (void)sm;
    if (file && seen.insert(file).second)
      out.emplace_back(pathFromFileEntry(*file), file->getModificationTime());
  }
};

class CclsPreambleCallbacks : public PreambleCallbacks {
public:
  void BeforeExecute(CompilerInstance &ci) override {
    sm = &ci.getSourceManager();
  }
  std::unique_ptr<PPCallbacks> createPPCallbacks() override {
    return std::make_unique<StoreInclude>(*sm, includes);
  }
  SourceManager *sm = nullptr;
  IncludeStructure includes;
};

class StoreDiags : public DiagnosticConsumer {
  const LangOptions *langOpts;
  std::optional<Diag> last;
  std::vector<Diag> output;
  std::string path;
  std::unordered_map<unsigned, bool> fID2concerned;
  void flush() {
    if (!last)
      return;
    bool mentions = last->concerned || last->edits.size();
    if (!mentions)
      for (auto &n : last->notes)
        if (n.concerned)
          mentions = true;
    if (mentions)
      output.push_back(std::move(*last));
    last.reset();
  }

public:
  StoreDiags(std::string path) : path(std::move(path)) {}
  std::vector<Diag> take() { return std::move(output); }
  bool isConcerned(const SourceManager &sm, SourceLocation l) {
    FileID fid = sm.getFileID(l);
    auto it = fID2concerned.try_emplace(fid.getHashValue());
    if (it.second) {
      const FileEntry *fe = sm.getFileEntryForID(fid);
      it.first->second = fe && pathFromFileEntry(*fe) == path;
    }
    return it.first->second;
  }
  void BeginSourceFile(const LangOptions &opts, const Preprocessor *) override {
    langOpts = &opts;
  }
  void EndSourceFile() override { flush(); }
  void HandleDiagnostic(DiagnosticsEngine::Level level,
                        const clang::Diagnostic &info) override {
    DiagnosticConsumer::HandleDiagnostic(level, info);
    SourceLocation l = info.getLocation();
    if (!l.isValid())
      return;
    const SourceManager &sm = info.getSourceManager();
    StringRef filename = sm.getFilename(info.getLocation());
    bool concerned = isInsideMainFile(sm, l);
    auto fillDiagBase = [&](DiagBase &d) {
      llvm::SmallString<64> message;
      info.FormatDiagnostic(message);
      d.range =
          fromCharSourceRange(sm, *langOpts, diagnosticRange(info, *langOpts));
      d.message = message.str();
      d.concerned = concerned;
      d.file = filename;
      d.level = level;
      d.category = DiagnosticIDs::getCategoryNumberForDiag(info.getID());
    };

    auto addFix = [&](bool syntheticMessage) -> bool {
      if (!concerned)
        return false;
      for (const FixItHint &fixIt : info.getFixItHints()) {
        if (!isConcerned(sm, fixIt.RemoveRange.getBegin()))
          return false;
        last->edits.push_back(toTextEdit(sm, *langOpts, fixIt));
      }
      return true;
    };

    if (level == DiagnosticsEngine::Note ||
        level == DiagnosticsEngine::Remark) {
      if (info.getFixItHints().size()) {
        addFix(false);
      } else {
        Note &n = last->notes.emplace_back();
        fillDiagBase(n);
        if (concerned)
          last->concerned = true;
      }
    } else {
      flush();
      last = Diag();
      fillDiagBase(*last);
      if (!info.getFixItHints().empty())
        addFix(true);
    }
  }
};

std::unique_ptr<CompilerInstance>
buildCompilerInstance(Session &session, std::unique_ptr<CompilerInvocation> ci,
                      IntrusiveRefCntPtr<llvm::vfs::FileSystem> fs,
                      DiagnosticConsumer &dc, const PreambleData *preamble,
                      const std::string &main,
                      std::unique_ptr<llvm::MemoryBuffer> &buf) {
  if (preamble)
    preamble->preamble.OverridePreamble(*ci, fs, buf.get());
  else
    ci->getPreprocessorOpts().addRemappedFile(main, buf.get());

  auto clang = std::make_unique<CompilerInstance>(session.pch);
  clang->setInvocation(std::move(ci));
  clang->createDiagnostics(&dc, false);
  clang->setTarget(TargetInfo::CreateTargetInfo(
      clang->getDiagnostics(), clang->getInvocation().TargetOpts));
  if (!clang->hasTarget())
    return nullptr;
  clang->getPreprocessorOpts().RetainRemappedFileBuffers = true;
  // Construct SourceManager with UserFilesAreVolatile: true because otherwise
  // RequiresNullTerminator: true may cause out-of-bounds read when a file is
  // mmap'ed but is saved concurrently.
#if LLVM_VERSION_MAJOR >= 9 // rC357037
  clang->createFileManager(fs);
#else
  clang->setVirtualFileSystem(fs);
  clang->createFileManager();
#endif
  clang->setSourceManager(new SourceManager(clang->getDiagnostics(),
                                            clang->getFileManager(), true));
  auto &isec = clang->getFrontendOpts().Inputs;
  if (isec.size()) {
    assert(isec[0].isFile());
    isec[0] = FrontendInputFile(main, isec[0].getKind(), isec[0].isSystem());
  }
  return clang;
}

bool parse(CompilerInstance &clang) {
  SyntaxOnlyAction action;
  if (!action.BeginSourceFile(clang, clang.getFrontendOpts().Inputs[0]))
    return false;
#if LLVM_VERSION_MAJOR >= 9 // rL364464
  if (llvm::Error e = action.Execute()) {
    llvm::consumeError(std::move(e));
    return false;
  }
#else
  if (!action.Execute())
    return false;
#endif
  action.EndSourceFile();
  return true;
}

void buildPreamble(Session &session, CompilerInvocation &ci,
                   IntrusiveRefCntPtr<llvm::vfs::FileSystem> fs,
                   const SemaManager::PreambleTask &task,
                   std::unique_ptr<PreambleStatCache> stat_cache) {
  std::shared_ptr<PreambleData> oldP = session.getPreamble();
  std::string content = session.wfiles->getContent(task.path);
  std::unique_ptr<llvm::MemoryBuffer> buf =
      llvm::MemoryBuffer::getMemBuffer(content);
  auto bounds = ComputePreambleBounds(*ci.getLangOpts(), buf.get(), 0);
  if (!task.from_diag && oldP &&
      oldP->preamble.CanReuse(ci, buf.get(), bounds, fs.get()))
    return;
  // -Werror makes warnings issued as errors, which stops parsing
  // prematurely because of -ferror-limit=. This also works around the issue
  // of -Werror + -Wunused-parameter in interaction with SkipFunctionBodies.
  auto &ws = ci.getDiagnosticOpts().Warnings;
  ws.erase(std::remove(ws.begin(), ws.end(), "error"), ws.end());
  ci.getDiagnosticOpts().IgnoreWarnings = false;
  ci.getFrontendOpts().SkipFunctionBodies = true;
  ci.getLangOpts()->CommentOpts.ParseAllComments = g_config->index.comments > 1;
  ci.getLangOpts()->RetainCommentsFromSystemHeaders = true;

  StoreDiags dc(task.path);
  IntrusiveRefCntPtr<DiagnosticsEngine> de =
      CompilerInstance::createDiagnostics(&ci.getDiagnosticOpts(), &dc, false);
  if (oldP) {
    std::lock_guard lock(session.wfiles->mutex);
    for (auto &include : oldP->includes)
      if (WorkingFile *wf = session.wfiles->getFileUnlocked(include.first))
        ci.getPreprocessorOpts().addRemappedFile(
            include.first,
            llvm::MemoryBuffer::getMemBufferCopy(wf->buffer_content).release());
  }

  CclsPreambleCallbacks pc;
  if (auto newPreamble = PrecompiledPreamble::Build(
          ci, buf.get(), bounds, *de, fs, session.pch, true, pc)) {
    assert(!ci.getPreprocessorOpts().RetainRemappedFileBuffers);
    if (oldP) {
      auto &old_includes = oldP->includes;
      auto it = old_includes.begin();
      std::sort(pc.includes.begin(), pc.includes.end());
      for (auto &include : pc.includes)
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
        std::move(*newPreamble), std::move(pc.includes), dc.take(),
        std::move(stat_cache));
  }
}

void *preambleMain(void *manager_) {
  auto *manager = static_cast<SemaManager *>(manager_);
  set_thread_name("preamble");
  while (true) {
    SemaManager::PreambleTask task = manager->preamble_tasks.dequeue();
    if (pipeline::g_quit.load(std::memory_order_relaxed))
      break;

    bool created = false;
    std::shared_ptr<Session> session =
        manager->ensureSession(task.path, &created);

    auto stat_cache = std::make_unique<PreambleStatCache>();
    IntrusiveRefCntPtr<llvm::vfs::FileSystem> fs =
        stat_cache->producer(session->fs);
    if (std::unique_ptr<CompilerInvocation> ci =
            buildCompilerInvocation(task.path, session->file.args, fs))
      buildPreamble(*session, *ci, fs, task, std::move(stat_cache));

    if (task.comp_task) {
      manager->comp_tasks.pushBack(std::move(task.comp_task));
    } else if (task.from_diag) {
      manager->scheduleDiag(task.path, 0);
    } else {
      int debounce =
          created ? g_config->diagnostics.onOpen : g_config->diagnostics.onSave;
      if (debounce >= 0)
        manager->scheduleDiag(task.path, debounce);
    }
  }
  pipeline::threadLeave();
  return nullptr;
}

void *completionMain(void *manager_) {
  auto *manager = static_cast<SemaManager *>(manager_);
  set_thread_name("comp");
  while (true) {
    std::unique_ptr<SemaManager::CompTask> task = manager->comp_tasks.dequeue();
    if (pipeline::g_quit.load(std::memory_order_relaxed))
      break;

    // Drop older requests if we're not buffering.
    while (g_config->completion.dropOldRequests &&
           !manager->comp_tasks.isEmpty()) {
      manager->on_dropped_(task->id);
      task->consumer.reset();
      task->on_complete(nullptr);
      task = manager->comp_tasks.dequeue();
      if (pipeline::g_quit.load(std::memory_order_relaxed))
        break;
    }

    std::shared_ptr<Session> session = manager->ensureSession(task->path);
    std::shared_ptr<PreambleData> preamble = session->getPreamble();
    IntrusiveRefCntPtr<llvm::vfs::FileSystem> fs =
        preamble ? preamble->stat_cache->consumer(session->fs) : session->fs;
    std::unique_ptr<CompilerInvocation> ci =
        buildCompilerInvocation(task->path, session->file.args, fs);
    if (!ci)
      continue;
    auto &fOpts = ci->getFrontendOpts();
    fOpts.CodeCompleteOpts = task->cc_opts;
    fOpts.CodeCompletionAt.FileName = task->path;
    fOpts.CodeCompletionAt.Line = task->position.line + 1;
    fOpts.CodeCompletionAt.Column = task->position.character + 1;
    ci->getLangOpts()->CommentOpts.ParseAllComments = true;

    DiagnosticConsumer dc;
    std::string content = manager->wfiles->getContent(task->path);
    auto buf = llvm::MemoryBuffer::getMemBuffer(content);
    PreambleBounds bounds =
        ComputePreambleBounds(*ci->getLangOpts(), buf.get(), 0);
    bool in_preamble =
        getOffsetForPosition({task->position.line, task->position.character},
                             content) < (int)bounds.Size;
    if (in_preamble) {
      preamble.reset();
    } else if (preamble && bounds.Size != preamble->preamble.getBounds().Size) {
      manager->preamble_tasks.pushBack({task->path, std::move(task), false},
                                       true);
      continue;
    }
    auto clang = buildCompilerInstance(*session, std::move(ci), fs, dc,
                                       preamble.get(), task->path, buf);
    if (!clang)
      continue;

    clang->getPreprocessorOpts().SingleFileParseMode = in_preamble;
    clang->setCodeCompletionConsumer(task->consumer.release());
    if (!parse(*clang))
      continue;

    task->on_complete(&clang->getCodeCompletionConsumer());
  }
  pipeline::threadLeave();
  return nullptr;
}

llvm::StringRef diagLeveltoString(DiagnosticsEngine::Level lvl) {
  switch (lvl) {
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

void printDiag(llvm::raw_string_ostream &os, const DiagBase &d) {
  if (d.concerned)
    os << llvm::sys::path::filename(d.file);
  else
    os << d.file;
  auto pos = d.range.start;
  os << ":" << (pos.line + 1) << ":" << (pos.column + 1) << ":"
     << (d.concerned ? " " : "\n");
  os << diagLeveltoString(d.level) << ": " << d.message;
}

void *diagnosticMain(void *manager_) {
  auto *manager = static_cast<SemaManager *>(manager_);
  set_thread_name("diag");
  while (true) {
    SemaManager::DiagTask task = manager->diag_tasks.dequeue();
    if (pipeline::g_quit.load(std::memory_order_relaxed))
      break;
    int64_t wait = task.wait_until -
                   chrono::duration_cast<chrono::milliseconds>(
                       chrono::high_resolution_clock::now().time_since_epoch())
                       .count();
    if (wait > 0)
      std::this_thread::sleep_for(
          chrono::duration<int64_t, std::milli>(std::min(wait, task.debounce)));

    std::shared_ptr<Session> session = manager->ensureSession(task.path);
    std::shared_ptr<PreambleData> preamble = session->getPreamble();
    IntrusiveRefCntPtr<llvm::vfs::FileSystem> fs =
        preamble ? preamble->stat_cache->consumer(session->fs) : session->fs;
    if (preamble) {
      bool rebuild = false;
      {
        std::lock_guard lock(manager->wfiles->mutex);
        for (auto &include : preamble->includes)
          if (WorkingFile *wf = manager->wfiles->getFileUnlocked(include.first);
              wf && include.second < wf->timestamp) {
            include.second = wf->timestamp;
            rebuild = true;
          }
      }
      if (rebuild) {
        manager->preamble_tasks.pushBack({task.path, nullptr, true}, true);
        continue;
      }
    }

    std::unique_ptr<CompilerInvocation> ci =
        buildCompilerInvocation(task.path, session->file.args, fs);
    if (!ci)
      continue;
    // If main file is a header, add -Wno-unused-function
    if (lookupExtension(session->file.filename).second)
      ci->getDiagnosticOpts().Warnings.push_back("no-unused-function");
    ci->getDiagnosticOpts().IgnoreWarnings = false;
    ci->getFrontendOpts().SkipFunctionBodies = false;
    ci->getLangOpts()->SpellChecking = g_config->diagnostics.spellChecking;
    StoreDiags dc(task.path);
    std::string content = manager->wfiles->getContent(task.path);
    auto buf = llvm::MemoryBuffer::getMemBuffer(content);
    auto clang = buildCompilerInstance(*session, std::move(ci), fs, dc,
                                       preamble.get(), task.path, buf);
    if (!clang)
      continue;
    if (!parse(*clang))
      continue;

    auto fill = [](const DiagBase &d, Diagnostic &ret) {
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
      ret.code = (int)d.category;
      return ret;
    };

    std::vector<Diag> diags = dc.take();
    if (std::shared_ptr<PreambleData> preamble = session->getPreamble())
      diags.insert(diags.end(), preamble->diags.begin(), preamble->diags.end());
    std::vector<Diagnostic> ls_diags;
    for (auto &d : diags) {
      if (!d.concerned)
        continue;
      Diagnostic &ls_diag = ls_diags.emplace_back();
      fill(d, ls_diag);
      ls_diag.fixits_ = d.edits;
      if (g_config->client.diagnosticsRelatedInformation) {
        ls_diag.message = d.message;
        for (const Note &n : d.notes) {
          SmallString<256> str(n.file);
          llvm::sys::path::remove_dots(str, true);
          Location loc{
              DocumentUri::fromPath(std::string(str.data(), str.size())),
              lsRange{{n.range.start.line, n.range.start.column},
                      {n.range.end.line, n.range.end.column}}};
          ls_diag.relatedInformation.push_back({loc, n.message});
        }
      } else {
        std::string buf;
        llvm::raw_string_ostream os(buf);
        os << d.message;
        for (const Note &n : d.notes) {
          os << "\n\n";
          printDiag(os, n);
        }
        os.flush();
        ls_diag.message = std::move(buf);
        for (const Note &n : d.notes) {
          if (!n.concerned)
            continue;
          Diagnostic &ls_diag1 = ls_diags.emplace_back();
          fill(n, ls_diag1);
          buf.clear();
          os << n.message << "\n\n";
          printDiag(os, d);
          os.flush();
          ls_diag1.message = std::move(buf);
        }
      }
    }

    {
      std::lock_guard lock(manager->wfiles->mutex);
      if (WorkingFile *wf = manager->wfiles->getFileUnlocked(task.path))
        wf->diagnostics = ls_diags;
    }
    manager->on_diagnostic_(task.path, ls_diags);
  }
  pipeline::threadLeave();
  return nullptr;
}

} // namespace

std::shared_ptr<PreambleData> Session::getPreamble() {
  std::lock_guard<std::mutex> lock(mutex);
  return preamble;
}

SemaManager::SemaManager(Project *project, WorkingFiles *wfiles,
                         OnDiagnostic on_diagnostic, OnDropped on_dropped)
    : project_(project), wfiles(wfiles),
      on_diagnostic_(std::move(on_diagnostic)),
      on_dropped_(std::move(on_dropped)),
      pch(std::make_shared<PCHContainerOperations>()) {
  spawnThread(ccls::preambleMain, this);
  spawnThread(ccls::completionMain, this);
  spawnThread(ccls::diagnosticMain, this);
}

void SemaManager::scheduleDiag(const std::string &path, int debounce) {
  static GroupMatch match(g_config->diagnostics.whitelist,
                          g_config->diagnostics.blacklist);
  if (!match.matches(path))
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
    diag_tasks.pushBack({path, now + debounce, debounce}, false);
}

void SemaManager::onView(const std::string &path) {
  std::lock_guard lock(mutex);
  if (!sessions.get(path))
    preamble_tasks.pushBack(PreambleTask{path}, true);
}

void SemaManager::onSave(const std::string &path) {
  preamble_tasks.pushBack(PreambleTask{path}, true);
}

void SemaManager::onClose(const std::string &path) {
  std::lock_guard lock(mutex);
  sessions.take(path);
}

std::shared_ptr<ccls::Session>
SemaManager::ensureSession(const std::string &path, bool *created) {
  std::lock_guard lock(mutex);
  std::shared_ptr<ccls::Session> session = sessions.get(path);
  if (!session) {
    session = std::make_shared<ccls::Session>(
        project_->findEntry(path, false, false), wfiles, pch);
    std::string line;
    if (LOG_V_ENABLED(1)) {
      line = "\n ";
      for (auto &arg : session->file.args)
        (line += ' ') += arg;
    }
    LOG_S(INFO) << "create session for " << path << line;
    sessions.insert(path, session);
    if (created)
      *created = true;
  }
  return session;
}

void SemaManager::clear() {
  LOG_S(INFO) << "clear all sessions";
  std::lock_guard lock(mutex);
  sessions.clear();
}

void SemaManager::quit() {
  comp_tasks.pushBack(nullptr);
  diag_tasks.pushBack({});
  preamble_tasks.pushBack({});
}
} // namespace ccls
