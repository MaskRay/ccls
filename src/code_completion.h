#include "atomic_object.h"
#include "language_server_api.h"
#include "libclangmm/Index.h"
#include "libclangmm/TranslationUnit.h"
#include "project.h"
#include "threaded_queue.h"
#include "working_files.h"

#include <clang-c/Index.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>

// TODO: rename this file to clang_completion.h/cc

struct CompletionSession {
  Project::Entry file;
  WorkingFiles* working_files;
  clang::Index index;

  // When |tu| was last parsed.
  optional<std::chrono::time_point<std::chrono::high_resolution_clock>> tu_last_parsed_at;

  // Acquired when |tu| is being used.
  std::mutex tu_lock;

  // The active translation unit.
  std::unique_ptr<clang::TranslationUnit> tu;

  CompletionSession(const Project::Entry& file, WorkingFiles* working_files);
  ~CompletionSession();
};

struct LruSessionCache {
  std::vector<std::unique_ptr<CompletionSession>> entries_;
  int max_entries_;

  LruSessionCache(int max_entries);

  // Fetches the entry for |filename| and updates it's usage so it is less
  // likely to be evicted.
  CompletionSession* TryGetEntry(const std::string& filename);
  // Inserts an entry. Evicts the oldest unused entry if there is no space.
  void InsertEntry(std::unique_ptr<CompletionSession> session);
};

struct CompletionManager {
  using OnComplete = std::function<void(NonElidedVector<lsCompletionItem> results, NonElidedVector<lsDiagnostic> diagnostics)>;
  
  struct ParseRequest {
    ParseRequest(const std::string& path);

    std::chrono::time_point<std::chrono::high_resolution_clock> request_time;
    std::string path;
  };
  struct CompletionRequest {
    lsTextDocumentPositionParams location;
    OnComplete on_complete;
  };

  CompletionManager(Config* config, Project* project, WorkingFiles* working_files);

  // Start a code completion at the given location. |on_complete| will run when
  // completion results are available. |on_complete| may run on any thread.
  void CodeComplete(const lsTextDocumentPositionParams& completion_location,
                    const OnComplete& on_complete);

  // Notify the completion manager that |filename| has been viewed and we
  // should begin preloading completion data.
  void NotifyView(const std::string& filename);
  // Notify the completion manager that |filename| has been edited.
  void NotifyEdit(const std::string& filename);
  // Notify the completion manager that |filename| has been saved. This
  // triggers a reparse.
  void NotifySave(const std::string& filename);

  CompletionSession* TryGetSession(const std::string& filename, bool create_if_needed);

  // TODO: make these configurable.
  const int kMaxViewSessions = 3;
  const int kMaxEditSessions = 10;

  // Global state.
  Config* config_;
  Project* project_;
  WorkingFiles* working_files_;

  // Sessions which have never had a real text-edit applied, but are preloaded
  // to give a fast initial experience.
  LruSessionCache view_sessions_;
  // Completion sessions which have been edited.
  LruSessionCache edit_sessions_;
  // Mutex which protects |view_sessions_| and |edit_sessions_|.
  std::mutex sessions_lock_;

  // Request a code completion at the given location.
  AtomicObject<CompletionRequest> completion_request_;
  // Parse requests. The path may already be parsed, in which case it should be
  // reparsed.
  ThreadedQueue<ParseRequest> parse_requests_;
};