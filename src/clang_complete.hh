// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "clang_tu.hh"
#include "lru_cache.h"
#include "lsp_completion.h"
#include "lsp_diagnostic.h"
#include "project.h"
#include "threaded_queue.h"
#include "working_files.h"

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Sema/CodeCompleteOptions.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace ccls {
struct PreambleData;

struct DiagBase {
  Range range;
  std::string message;
  std::string file;
  clang::DiagnosticsEngine::Level level = clang::DiagnosticsEngine::Note;
  unsigned category;
  bool concerned = false;
};
struct Note : DiagBase {};
struct Diag : DiagBase {
  std::vector<Note> notes;
  std::vector<lsTextEdit> edits;
};

lsTextEdit ToTextEdit(const clang::SourceManager &SM,
                      const clang::LangOptions &L,
                      const clang::FixItHint &FixIt);

struct CompletionSession
    : public std::enable_shared_from_this<CompletionSession> {
  std::mutex mutex;
  std::shared_ptr<PreambleData> preamble;

  Project::Entry file;
  WorkingFiles *wfiles;
  bool inferred = false;

  // TODO share
  llvm::IntrusiveRefCntPtr<clang::vfs::FileSystem> FS =
      clang::vfs::getRealFileSystem();
  std::shared_ptr<clang::PCHContainerOperations> PCH;

  CompletionSession(const Project::Entry &file, WorkingFiles *wfiles,
                    std::shared_ptr<clang::PCHContainerOperations> PCH)
      : file(file), wfiles(wfiles), PCH(PCH) {}

  std::shared_ptr<PreambleData> GetPreamble();
};
}

struct CompletionManager {
  using OnDiagnostic = std::function<void(
      std::string path, std::vector<lsDiagnostic> diagnostics)>;
  // If OptConsumer is nullptr, the request has been cancelled.
  using OnComplete =
      std::function<void(clang::CodeCompleteConsumer *OptConsumer)>;
  using OnDropped = std::function<void(lsRequestId request_id)>;

  struct PreloadRequest {
    std::string path;
  };
  struct CompletionRequest {
    CompletionRequest(const lsRequestId &id,
                      const lsTextDocumentIdentifier &document,
                      const lsPosition &position,
                      std::unique_ptr<clang::CodeCompleteConsumer> Consumer,
                      clang::CodeCompleteOptions CCOpts,
                      const OnComplete &on_complete)
        : id(id), document(document), position(position),
          Consumer(std::move(Consumer)), CCOpts(CCOpts),
          on_complete(on_complete) {}

    lsRequestId id;
    lsTextDocumentIdentifier document;
    lsPosition position;
    std::unique_ptr<clang::CodeCompleteConsumer> Consumer;
    clang::CodeCompleteOptions CCOpts;
    OnComplete on_complete;
  };
  struct DiagnosticRequest {
    std::string path;
    int64_t wait_until;
    int64_t debounce;
  };

  CompletionManager(Project *project, WorkingFiles *working_files,
                    OnDiagnostic on_diagnostic, OnDropped on_dropped);

  // Request a diagnostics update.
  void DiagnosticsUpdate(const std::string &path, int debounce);

  // Notify the completion manager that |filename| has been viewed and we
  // should begin preloading completion data.
  void NotifyView(const std::string &path);
  // Notify the completion manager that |filename| has been saved. This
  // triggers a reparse.
  void NotifySave(const std::string &path);
  // Notify the completion manager that |filename| has been closed. Any existing
  // completion session will be dropped.
  void OnClose(const std::string &path);

  // Ensures there is a completion or preloaded session. Returns true if a new
  // session was created.
  bool EnsureCompletionOrCreatePreloadSession(const std::string &path);
  // Tries to find an edit session for |filename|. This will move the session
  // from view to edit.
  std::shared_ptr<ccls::CompletionSession>
  TryGetSession(const std::string &path, bool preload, bool *is_open = nullptr);

  // Flushes all saved sessions
  void FlushAllSessions(void);

  // TODO: make these configurable.
  const int kMaxPreloadedSessions = 10;
  const int kMaxCompletionSessions = 5;

  // Global state.
  Project *project_;
  WorkingFiles *working_files_;
  OnDiagnostic on_diagnostic_;
  OnDropped on_dropped_;

  using LruSessionCache = LruCache<std::string, ccls::CompletionSession>;

  // CompletionSession instances which are preloaded, ie, files which the user
  // has viewed but not requested code completion for.
  LruSessionCache preloads;
  // CompletionSession instances which the user has actually performed
  // completion on. This is more rare so these instances tend to stay alive
  // much longer than the ones in |preloaded_sessions_|.
  LruSessionCache sessions;
  // Mutex which protects |view_sessions_| and |edit_sessions_|.
  std::mutex sessions_lock_;

  std::mutex diag_mutex;
  std::unordered_map<std::string, int64_t> next_diag;

  // Request a code completion at the given location.
  ThreadedQueue<std::unique_ptr<CompletionRequest>> completion_request_;
  ThreadedQueue<DiagnosticRequest> diagnostic_request_;
  // Parse requests. The path may already be parsed, in which case it should be
  // reparsed.
  ThreadedQueue<PreloadRequest> preload_requests_;

  std::shared_ptr<clang::PCHContainerOperations> PCH;
};

// Cached completion information, so we can give fast completion results when
// the user erases a character. vscode will resend the completion request if
// that happens.
template <typename T>
struct CompleteConsumerCache {
  // NOTE: Make sure to access these variables under |WithLock|.
  std::optional<std::string> path;
  std::optional<lsPosition> position;
  T result;

  std::mutex mutex;

  void WithLock(std::function<void()> action) {
    std::lock_guard lock(mutex);
    action();
  }
  bool IsCacheValid(const std::string path, lsPosition position) {
    std::lock_guard lock(mutex);
    return this->path == path && this->position == position;
  }
};
