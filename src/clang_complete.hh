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

#pragma once

#include "clang_tu.h"
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
struct DiagBase {
  Range range;
  std::string message;
  std::string file;
  clang::DiagnosticsEngine::Level level = clang::DiagnosticsEngine::Note;
  unsigned category;
  bool inside_main = false;
};
struct Note : DiagBase {};
struct Diag : DiagBase {
  std::vector<Note> notes;
  std::vector<lsTextEdit> edits;
};

struct PreambleData {
  PreambleData(clang::PrecompiledPreamble P, std::vector<Diag> diags)
      : Preamble(std::move(P)), diags(std::move(diags)) {}
  clang::PrecompiledPreamble Preamble;
  std::vector<Diag> diags;
};

struct CompletionSession
    : public std::enable_shared_from_this<CompletionSession> {
  std::mutex mutex;
  std::shared_ptr<PreambleData> preamble;
  std::vector<Diag> diags;

  Project::Entry file;
  WorkingFiles *wfiles;

  // TODO share
  llvm::IntrusiveRefCntPtr<clang::vfs::FileSystem> FS =
      clang::vfs::getRealFileSystem();
  std::shared_ptr<clang::PCHContainerOperations> PCH;

  CompletionSession(const Project::Entry &file, WorkingFiles *wfiles,
                    std::shared_ptr<clang::PCHContainerOperations> PCH)
      : file(file), wfiles(wfiles), PCH(PCH) {}

  std::shared_ptr<PreambleData> GetPreamble();
  void BuildPreamble(clang::CompilerInvocation &CI);
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
    PreloadRequest(const std::string &path)
        : request_time(std::chrono::high_resolution_clock::now()), path(path) {}

    std::chrono::time_point<std::chrono::high_resolution_clock> request_time;
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
    lsTextDocumentIdentifier document;
  };

  CompletionManager(Project *project, WorkingFiles *working_files,
                    OnDiagnostic on_diagnostic, OnDropped on_dropped);

  // Start a code completion at the given location. |on_complete| will run when
  // completion results are available. |on_complete| may run on any thread.
  void CodeComplete(const lsRequestId &request_id,
                    const lsTextDocumentPositionParams &completion_location,
                    const OnComplete &on_complete);
  // Request a diagnostics update.
  void DiagnosticsUpdate(const lsTextDocumentIdentifier &document);

  // Notify the completion manager that |filename| has been viewed and we
  // should begin preloading completion data.
  void NotifyView(const std::string &path);
  // Notify the completion manager that |filename| has been saved. This
  // triggers a reparse.
  void NotifySave(const std::string &path);
  // Notify the completion manager that |filename| has been closed. Any existing
  // completion session will be dropped.
  void NotifyClose(const std::string &path);

  // Ensures there is a completion or preloaded session. Returns true if a new
  // session was created.
  bool EnsureCompletionOrCreatePreloadSession(const std::string &path);
  // Tries to find an edit session for |filename|. This will move the session
  // from view to edit.
  std::shared_ptr<ccls::CompletionSession>
  TryGetSession(const std::string &path, bool mark_as_completion,
                bool create_if_needed);

  // Flushes all saved sessions with the supplied filename
  void FlushSession(const std::string &path);
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
  LruSessionCache preloaded_sessions_;
  // CompletionSession instances which the user has actually performed
  // completion on. This is more rare so these instances tend to stay alive
  // much longer than the ones in |preloaded_sessions_|.
  LruSessionCache completion_sessions_;
  // Mutex which protects |view_sessions_| and |edit_sessions_|.
  std::mutex sessions_lock_;

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
  bool IsCacheValid(const lsTextDocumentPositionParams &params) {
    std::lock_guard lock(mutex);
    return path == params.textDocument.uri.GetPath() &&
           position == params.position;
  }
};
