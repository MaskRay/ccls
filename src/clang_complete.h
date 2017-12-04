#include "atomic_object.h"
#include "clang_index.h"
#include "clang_translation_unit.h"
#include "language_server_api.h"
#include "lru_cache.h"
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
  Project::Entry file;
  WorkingFiles* working_files;
  ClangIndex index;

  // When |tu| was last parsed.
  optional<std::chrono::time_point<std::chrono::high_resolution_clock>>
      tu_last_parsed_at;

  // Acquired when |tu| is being used.
  std::mutex tu_lock;

  // The active translation unit.
  std::unique_ptr<ClangTranslationUnit> tu;

  CompletionSession(const Project::Entry& file, WorkingFiles* working_files);
  ~CompletionSession();
};

struct ClangCompleteManager {
  using OnDiagnostic =
      std::function<void(std::string path,
                         NonElidedVector<lsDiagnostic> diagnostics)>;
  using OnIndex = std::function<void(ClangTranslationUnit* tu,
                                     const std::vector<CXUnsavedFile>& unsaved,
                                     const std::string& path,
                                     const std::vector<std::string>& args)>;
  using OnComplete =
      std::function<void(const NonElidedVector<lsCompletionItem>& results,
                         bool is_cached_result)>;

  struct ParseRequest {
    ParseRequest(const std::string& path);

    std::chrono::time_point<std::chrono::high_resolution_clock> request_time;
    std::string path;
  };
  struct CompletionRequest {
    lsTextDocumentIdentifier document;
    optional<lsPosition> position;
    OnComplete on_complete;  // May be null/empty.
    bool emit_diagnostics = false;
  };

  ClangCompleteManager(Config* config,
                       Project* project,
                       WorkingFiles* working_files,
                       OnDiagnostic on_diagnostic,
                       OnIndex on_index);
  ~ClangCompleteManager();

  // Start a code completion at the given location. |on_complete| will run when
  // completion results are available. |on_complete| may run on any thread.
  void CodeComplete(const lsTextDocumentPositionParams& completion_location,
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

  // TODO: make these configurable.
  const int kMaxPreloadedSessions = 10;
  const int kMaxCompletionSessions = 5;

  // Global state.
  Config* config_;
  Project* project_;
  WorkingFiles* working_files_;
  OnDiagnostic on_diagnostic_;
  OnIndex on_index_;

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
  AtomicObject<CompletionRequest> completion_request_;
  // Parse requests. The path may already be parsed, in which case it should be
  // reparsed.
  ThreadedQueue<ParseRequest> parse_requests_;
};
