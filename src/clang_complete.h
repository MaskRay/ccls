#pragma once

#include "clang_cursor.h"
#include "clang_translation_unit.h"
#include "lru_cache.h"
#include "lsp_completion.h"
#include "lsp_diagnostic.h"
#include "project.h"
#include "threaded_queue.h"
#include "working_files.h"

#include <clang-c/Index.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>

struct CompletionSession
    : public std::enable_shared_from_this<CompletionSession> {
  // Translation unit for clang.
  struct Tu {
    ClangIndex index{0, 0};

    // When |tu| was last parsed.
    std::optional<std::chrono::time_point<std::chrono::high_resolution_clock>>
        last_parsed_at;
    // Acquired when |tu| is being used.
    std::mutex lock;
    std::unique_ptr<ClangTranslationUnit> tu;
  };

  Project::Entry file;
  WorkingFiles* working_files;

  Tu completion;
  Tu diagnostics;

  CompletionSession(const Project::Entry& file, WorkingFiles* wfiles)
      : file(file), working_files(wfiles) {}
};

struct ClangCompleteManager {
  using OnDiagnostic =
      std::function<void(std::string path,
                         std::vector<lsDiagnostic> diagnostics)>;
  using OnComplete =
      std::function<void(const std::vector<lsCompletionItem>& results,
                         bool is_cached_result)>;
  using OnDropped = std::function<void(lsRequestId request_id)>;

  struct PreloadRequest {
    PreloadRequest(const std::string& path)
        : request_time(std::chrono::high_resolution_clock::now()), path(path) {}

    std::chrono::time_point<std::chrono::high_resolution_clock> request_time;
    std::string path;
  };
  struct CompletionRequest {
    CompletionRequest(const lsRequestId& id,
                      const lsTextDocumentIdentifier& document,
                      const lsPosition& position,
                      const OnComplete& on_complete)
        : id(id),
          document(document),
          position(position),
          on_complete(on_complete) {}

    lsRequestId id;
    lsTextDocumentIdentifier document;
    lsPosition position;
    OnComplete on_complete;
  };
  struct DiagnosticRequest {
    lsTextDocumentIdentifier document;
  };

  ClangCompleteManager(Project* project,
                       WorkingFiles* working_files,
                       OnDiagnostic on_diagnostic,
                       OnDropped on_dropped);

  // Start a code completion at the given location. |on_complete| will run when
  // completion results are available. |on_complete| may run on any thread.
  void CodeComplete(const lsRequestId& request_id,
                    const lsTextDocumentPositionParams& completion_location,
                    const OnComplete& on_complete);
  // Request a diagnostics update.
  void DiagnosticsUpdate(const lsTextDocumentIdentifier& document);

  // Notify the completion manager that |filename| has been viewed and we
  // should begin preloading completion data.
  void NotifyView(const std::string& filename);
  // Notify the completion manager that |filename| has been edited.
  void NotifyEdit(const std::string& filename);
  // Notify the completion manager that |filename| has been saved. This
  // triggers a reparse.
  void NotifySave(const std::string& filename);
  // Notify the completion manager that |filename| has been closed. Any existing
  // completion session will be dropped.
  void NotifyClose(const std::string& filename);

  // Ensures there is a completion or preloaded session. Returns true if a new
  // session was created.
  bool EnsureCompletionOrCreatePreloadSession(const std::string& filename);
  // Tries to find an edit session for |filename|. This will move the session
  // from view to edit.
  std::shared_ptr<CompletionSession> TryGetSession(const std::string& filename,
                                                   bool mark_as_completion,
                                                   bool create_if_needed);

  // Flushes all saved sessions with the supplied filename
  void FlushSession(const std::string& filename);
  // Flushes all saved sessions
  void FlushAllSessions(void);

  // TODO: make these configurable.
  const int kMaxPreloadedSessions = 10;
  const int kMaxCompletionSessions = 5;

  // Global state.
  Project* project_;
  WorkingFiles* working_files_;
  OnDiagnostic on_diagnostic_;
  OnDropped on_dropped_;

  using LruSessionCache = LruCache<std::string, CompletionSession>;

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
};

// Cached completion information, so we can give fast completion results when
// the user erases a character. vscode will resend the completion request if
// that happens.
struct CodeCompleteCache {
  // NOTE: Make sure to access these variables under |WithLock|.
  std::optional<std::string> cached_path_;
  std::optional<lsPosition> cached_completion_position_;
  std::vector<lsCompletionItem> cached_results_;

  std::mutex mutex_;

  void WithLock(std::function<void()> action);
  bool IsCacheValid(lsTextDocumentPositionParams position);
};
