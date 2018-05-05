#include "import_pipeline.h"

#include "cache_manager.h"
#include "config.h"
#include "diagnostics_engine.h"
#include "lsp.h"
#include "message_handler.h"
#include "platform.h"
#include "project.h"
#include "query_utils.h"
#include "queue_manager.h"
#include "timer.h"

#include <doctest/doctest.h>
#include <loguru.hpp>

#include <chrono>

namespace {

struct Out_Progress : public lsOutMessage<Out_Progress> {
  struct Params {
    int indexRequestCount = 0;
    int loadPreviousIndexCount = 0;
    int onIdMappedCount = 0;
    int onIndexedCount = 0;
    int activeThreads = 0;
  };
  std::string method = "$ccls/progress";
  Params params;
};
MAKE_REFLECT_STRUCT(Out_Progress::Params,
                    indexRequestCount,
                    loadPreviousIndexCount,
                    onIdMappedCount,
                    onIndexedCount,
                    activeThreads);
MAKE_REFLECT_STRUCT(Out_Progress, jsonrpc, method, params);

long long GetCurrentTimeInMilliseconds() {
  auto time_since_epoch = Timer::Clock::now().time_since_epoch();
  long long elapsed_milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch)
          .count();
  return elapsed_milliseconds;
}

struct ActiveThread {
  ActiveThread(ImportPipelineStatus* status)
      : status_(status) {
    if (g_config && g_config->progressReportFrequencyMs < 0)
      return;

    ++status_->num_active_threads;
  }
  ~ActiveThread() {
    if (g_config && g_config->progressReportFrequencyMs < 0)
      return;

    --status_->num_active_threads;
    EmitProgress();
  }

  // Send indexing progress to client if reporting is enabled.
  void EmitProgress() {
    auto* queue = QueueManager::instance();
    Out_Progress out;
    out.params.indexRequestCount = queue->index_request.Size();
    out.params.onIdMappedCount = queue->on_id_mapped.Size();
    out.params.onIndexedCount = queue->on_indexed.Size();
    out.params.activeThreads = status_->num_active_threads;

    // Ignore this progress update if the last update was too recent.
    if (g_config && g_config->progressReportFrequencyMs != 0) {
      // Make sure we output a status update if queue lengths are zero.
      bool all_zero = out.params.indexRequestCount == 0 &&
                      out.params.loadPreviousIndexCount == 0 &&
                      out.params.onIdMappedCount == 0 &&
                      out.params.onIndexedCount == 0 &&
                      out.params.activeThreads == 0;
      if (!all_zero &&
          GetCurrentTimeInMilliseconds() < status_->next_progress_output)
        return;
      status_->next_progress_output =
          GetCurrentTimeInMilliseconds() + g_config->progressReportFrequencyMs;
    }

    QueueManager::WriteStdout(kMethodType_Unknown, out);
  }

  ImportPipelineStatus* status_;
};

enum class ShouldParse { Yes, No, NoSuchFile };

// Checks if |path| needs to be reparsed. This will modify cached state
// such that calling this function twice with the same path may return true
// the first time but will return false the second.
//
// |from|: The file which generated the parse request for this file.
ShouldParse FileNeedsParse(
    bool is_interactive,
    TimestampManager* timestamp_manager,
    const std::shared_ptr<ICacheManager>& cache_manager,
    IndexFile* opt_previous_index,
    const std::string& path,
    const std::vector<std::string>& args,
    const std::optional<std::string>& from) {
  auto unwrap_opt = [](const std::optional<std::string>& opt) -> std::string {
    if (opt)
      return " (via " + *opt + ")";
    return "";
  };

  std::optional<int64_t> modification_timestamp = LastWriteTime(path);

  // Cannot find file.
  if (!modification_timestamp)
    return ShouldParse::NoSuchFile;

  std::optional<int64_t> last_cached_modification =
      timestamp_manager->GetLastCachedModificationTime(cache_manager.get(),
                                                       path);

  // File has been changed.
  if (!last_cached_modification ||
      modification_timestamp != *last_cached_modification) {
    LOG_S(INFO) << "Timestamp has changed for " << path << unwrap_opt(from);
    return ShouldParse::Yes;
  }

  // Command-line arguments changed.
  auto is_file = [](const std::string& arg) {
    return EndsWithAny(arg, {".h", ".c", ".cc", ".cpp", ".hpp", ".m", ".mm"});
  };
  if (opt_previous_index) {
    auto& prev_args = opt_previous_index->args;
    bool same = prev_args.size() == args.size();
    for (size_t i = 0; i < args.size() && same; ++i) {
      same = prev_args[i] == args[i] ||
             (is_file(prev_args[i]) && is_file(args[i]));
    }
    if (!same) {
      LOG_S(INFO) << "Arguments have changed for " << path << unwrap_opt(from);
      return ShouldParse::Yes;
    }
  }

  // File has not changed, do not parse it.
  return ShouldParse::No;
};

enum CacheLoadResult { Parse, DoNotParse };
CacheLoadResult TryLoadFromCache(
    FileConsumerSharedState* file_consumer_shared,
    TimestampManager* timestamp_manager,
    const std::shared_ptr<ICacheManager>& cache_manager,
    bool is_interactive,
    const Project::Entry& entry,
    const std::string& path_to_index) {
  // Always run this block, even if we are interactive, so we can check
  // dependencies and reset files in |file_consumer_shared|.
  IndexFile* previous_index = cache_manager->TryLoad(path_to_index);
  if (!previous_index)
    return CacheLoadResult::Parse;

  // If none of the dependencies have changed and the index is not
  // interactive (ie, requested by a file save), skip parsing and just load
  // from cache.

  // Check timestamps and update |file_consumer_shared|.
  ShouldParse path_state =
      FileNeedsParse(is_interactive, timestamp_manager, cache_manager,
                     previous_index, path_to_index, entry.args, std::nullopt);
  if (path_state == ShouldParse::Yes)
    file_consumer_shared->Reset(path_to_index);

  // Target file does not exist on disk, do not emit any indexes.
  // TODO: Dependencies should be reassigned to other files. We can do this by
  // updating the "primary_file" if it doesn't exist. Might not actually be a
  // problem in practice.
  if (path_state == ShouldParse::NoSuchFile)
    return CacheLoadResult::DoNotParse;

  bool needs_reparse = is_interactive || path_state == ShouldParse::Yes;

  for (const std::string& dependency : previous_index->dependencies) {
    assert(!dependency.empty());

    if (FileNeedsParse(is_interactive, timestamp_manager, cache_manager,
                       previous_index, dependency, entry.args,
                       previous_index->path) == ShouldParse::Yes) {
      needs_reparse = true;

      // Do not break here, as we need to update |file_consumer_shared| for
      // every dependency that needs to be reparsed.
      file_consumer_shared->Reset(dependency);
    }
  }

  // FIXME: should we still load from cache?
  if (needs_reparse)
    return CacheLoadResult::Parse;

  // No timestamps changed - load directly from cache.
  LOG_S(INFO) << "load index for " << path_to_index;

  // TODO/FIXME: real perf
  PerformanceImportFile perf;

  std::vector<Index_OnIdMapped> result;
  result.push_back(Index_OnIdMapped(
      cache_manager, nullptr, cache_manager->TryTakeOrLoad(path_to_index), perf,
      is_interactive, false /*write_to_disk*/));
  for (const std::string& dependency : previous_index->dependencies) {
    // Only load a dependency if it is not already loaded.
    //
    // This is important for perf in large projects where there are lots of
    // dependencies shared between many files.
    if (!file_consumer_shared->Mark(dependency))
      continue;

    LOG_S(INFO) << "emit index for " << dependency << " via "
                << previous_index->path;

    // |dependency_index| may be null if there is no cache for it but
    // another file has already started importing it.
    if (std::unique_ptr<IndexFile> dependency_index =
            cache_manager->TryTakeOrLoad(dependency)) {
      result.push_back(
          Index_OnIdMapped(cache_manager, nullptr, std::move(dependency_index),
                           perf, is_interactive, false /*write_to_disk*/));
    }
  }

  QueueManager::instance()->on_id_mapped.EnqueueAll(std::move(result));
  return CacheLoadResult::DoNotParse;
}

std::vector<FileContents> PreloadFileContents(
    const std::shared_ptr<ICacheManager>& cache_manager,
    const Project::Entry& entry,
    const std::string& entry_contents,
    const std::string& path_to_index) {
  // Load file contents for all dependencies into memory. If the dependencies
  // for the file changed we may not end up using all of the files we
  // preloaded. If a new dependency was added the indexer will grab the file
  // contents as soon as possible.
  //
  // We do this to minimize the race between indexing a file and capturing the
  // file contents.
  //
  // TODO: We might be able to optimize perf by only copying for files in
  //       working_files. We can pass that same set of files to the indexer as
  //       well. We then default to a fast file-copy if not in working set.

  // index->file_contents comes from cache, so we need to check if that cache is
  // still valid. if so, we can use it, otherwise we need to load from disk.
  auto get_latest_content = [](const std::string& path, int64_t cached_time,
                               const std::string& cached) -> std::string {
    std::optional<int64_t> mod_time = LastWriteTime(path);
    if (!mod_time)
      return "";

    if (*mod_time == cached_time)
      return cached;

    std::optional<std::string> fresh_content = ReadContent(path);
    if (!fresh_content) {
      LOG_S(ERROR) << "Failed to load content for " << path;
      return "";
    }
    return *fresh_content;
  };

  std::vector<FileContents> file_contents;
  file_contents.push_back(FileContents(entry.filename, entry_contents));
  cache_manager->IterateLoadedCaches([&](IndexFile* index) {
    if (index->path == entry.filename)
      return;
    file_contents.push_back(FileContents(
        index->path,
        get_latest_content(index->path, index->last_modification_time,
                           index->file_contents)));
  });

  return file_contents;
}

void ParseFile(DiagnosticsEngine* diag_engine,
               WorkingFiles* working_files,
               FileConsumerSharedState* file_consumer_shared,
               TimestampManager* timestamp_manager,
               IIndexer* indexer,
               const Index_Request& request,
               const Project::Entry& entry) {
  // If the file is inferred, we may not actually be able to parse that file
  // directly (ie, a header file, which are not listed in the project). If this
  // file is inferred, then try to use the file which originally imported it.
  std::string path_to_index = entry.filename;
  if (entry.is_inferred) {
    IndexFile* entry_cache = request.cache_manager->TryLoad(entry.filename);
    if (entry_cache)
      path_to_index = entry_cache->import_file;
  }

  // Try to load the file from cache.
  if (TryLoadFromCache(file_consumer_shared, timestamp_manager,
                       request.cache_manager, request.is_interactive, entry,
                       path_to_index) == CacheLoadResult::DoNotParse)
    return;

  LOG_S(INFO) << "parse " << path_to_index;
  std::vector<FileContents> file_contents = PreloadFileContents(
      request.cache_manager, entry, request.contents, path_to_index);

  std::vector<Index_OnIdMapped> result;
  PerformanceImportFile perf;
  auto indexes = indexer->Index(file_consumer_shared, path_to_index, entry.args,
                                file_contents, &perf);

  if (indexes.empty()) {
    if (g_config->index.enabled && request.id.Valid()) {
      Out_Error out;
      out.id = request.id;
      out.error.code = lsErrorCodes::InternalError;
      out.error.message = "Failed to index " + path_to_index;
      QueueManager::WriteStdout(kMethodType_Unknown, out);
    }
    return;
  }

  for (std::unique_ptr<IndexFile>& new_index : indexes) {
    Timer time;

    // Only emit diagnostics for non-interactive sessions, which makes it easier
    // to identify indexing problems. For interactive sessions, diagnostics are
    // handled by code completion.
    if (!request.is_interactive)
      diag_engine->Publish(working_files, new_index->path,
                           new_index->diagnostics_);

    // When main thread does IdMap request it will request the previous index if
    // needed.
    LOG_S(INFO) << "emit index for " << new_index->path;
    result.push_back(
        Index_OnIdMapped(request.cache_manager,
                         request.cache_manager->TryTakeOrLoad(path_to_index),
                         std::move(new_index), perf, request.is_interactive,
                         true /*write_to_disk*/));
  }

  QueueManager::instance()->on_id_mapped.EnqueueAll(std::move(result),
                                                    request.is_interactive);
}

bool IndexMain_DoParse(
    DiagnosticsEngine* diag_engine,
    WorkingFiles* working_files,
    FileConsumerSharedState* file_consumer_shared,
    TimestampManager* timestamp_manager,
    IIndexer* indexer) {
  auto* queue = QueueManager::instance();
  std::optional<Index_Request> request = queue->index_request.TryPopFront();
  if (!request)
    return false;

  Project::Entry entry;
  entry.filename = request->path;
  entry.args = request->args;
  ParseFile(diag_engine, working_files, file_consumer_shared, timestamp_manager,
            indexer, request.value(), entry);
  return true;
}

bool IndexMain_DoCreateIndexUpdate(TimestampManager* timestamp_manager) {
  auto* queue = QueueManager::instance();

  bool did_work = false;
  for (int i = 100; i--; ) {
    std::optional<Index_OnIdMapped> response = queue->on_id_mapped.TryPopFront();
    if (!response)
      return did_work;

    did_work = true;

    Timer time;

    // Write current index to disk if requested.
    std::string path = response->current->path;
    if (response->write_to_disk) {
      LOG_S(INFO) << "store index for " << path;
      time.Reset();
      response->cache_manager->WriteToCache(*response->current);
      response->perf.index_save_to_disk = time.ElapsedMicrosecondsAndReset();
      timestamp_manager->UpdateCachedModificationTime(
          path, response->current->last_modification_time);
    }

    // Build delta update.
    IndexUpdate update = IndexUpdate::CreateDelta(response->previous.get(),
                                                  response->current.get());
    response->perf.index_make_delta = time.ElapsedMicrosecondsAndReset();
    LOG_S(INFO) << "built index for " << path
                << " (is_delta=" << !!response->previous << ")";

    Index_OnIndexed reply(std::move(update), response->perf);
    queue->on_indexed.PushBack(std::move(reply), response->is_interactive);
  }

  return did_work;
}

}  // namespace

std::optional<int64_t> TimestampManager::GetLastCachedModificationTime(
    ICacheManager* cache_manager,
    const std::string& path) {
  {
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = timestamps_.find(path);
    if (it != timestamps_.end())
      return it->second;
  }
  IndexFile* file = cache_manager->TryLoad(path);
  if (!file)
    return std::nullopt;

  UpdateCachedModificationTime(path, file->last_modification_time);
  return file->last_modification_time;
}

void TimestampManager::UpdateCachedModificationTime(const std::string& path,
                                                    int64_t timestamp) {
  std::lock_guard<std::mutex> guard(mutex_);
  timestamps_[path] = timestamp;
}

ImportPipelineStatus::ImportPipelineStatus()
    : num_active_threads(0), next_progress_output(0) {}

// Index a file using an already-parsed translation unit from code completion.
// Since most of the time for indexing a file comes from parsing, we can do
// real-time indexing.
// TODO: add option to disable this.
void IndexWithTuFromCodeCompletion(
    FileConsumerSharedState* file_consumer_shared,
    ClangTranslationUnit* tu,
    const std::vector<CXUnsavedFile>& file_contents,
    const std::string& path,
    const std::vector<std::string>& args) {
  file_consumer_shared->Reset(path);

  PerformanceImportFile perf;
  ClangIndex index;
  auto indexes = ParseWithTu(file_consumer_shared, &perf, tu, &index, path,
                             args, file_contents);
  if (indexes.empty())
    return;

  std::vector<Index_OnIdMapped> result;
  for (std::unique_ptr<IndexFile>& new_index : indexes) {
    Timer time;

    std::shared_ptr<ICacheManager> cache_manager;
    assert(false && "FIXME cache_manager");
    // When main thread does IdMap request it will request the previous index if
    // needed.
    LOG_S(INFO) << "Emitting index for " << new_index->path;
    result.push_back(Index_OnIdMapped(
        cache_manager, cache_manager->TryTakeOrLoad(path), std::move(new_index),
        perf, true /*is_interactive*/, true /*write_to_disk*/));
  }

  LOG_IF_S(WARNING, result.size() > 1)
      << "Code completion index update generated more than one index";

  QueueManager::instance()->on_id_mapped.EnqueueAll(std::move(result));
}

void Indexer_Main(DiagnosticsEngine* diag_engine,
                  FileConsumerSharedState* file_consumer_shared,
                  TimestampManager* timestamp_manager,
                  ImportPipelineStatus* status,
                  Project* project,
                  WorkingFiles* working_files,
                  MultiQueueWaiter* waiter) {
  auto* queue = QueueManager::instance();
  // Build one index per-indexer, as building the index acquires a global lock.
  auto indexer = std::make_unique<ClangIndexer>();

  while (true) {
    bool did_work = false;

    {
      ActiveThread active_thread(status);

      // TODO: process all off IndexMain_DoIndex before calling
      // IndexMain_DoCreateIndexUpdate for better icache behavior. We need to
      // have some threads spinning on both though otherwise memory usage will
      // get bad.

      // We need to make sure to run both IndexMain_DoParse and
      // IndexMain_DoCreateIndexUpdate so we don't starve querydb from doing any
      // work. Running both also lets the user query the partially constructed
      // index.
      did_work = IndexMain_DoParse(diag_engine, working_files,
                                   file_consumer_shared, timestamp_manager,
                                   indexer.get()) ||
                 did_work;

      did_work = IndexMain_DoCreateIndexUpdate(timestamp_manager) || did_work;
    }

    // We didn't do any work, so wait for a notification.
    if (!did_work) {
      waiter->Wait(&queue->on_indexed, &queue->index_request,
                   &queue->on_id_mapped);
    }
  }
}

namespace {
void QueryDb_OnIndexed(QueueManager* queue,
                       QueryDatabase* db,
                       ImportPipelineStatus* status,
                       SemanticHighlightSymbolCache* semantic_cache,
                       WorkingFiles* working_files,
                       Index_OnIndexed* response) {
  Timer time;
  db->ApplyIndexUpdate(&response->update);

  // Update indexed content, inactive lines, and semantic highlighting.
  if (response->update.files_def_update) {
    auto& update = *response->update.files_def_update;
    time.ResetAndPrint("apply index for " + update.value.path);
    WorkingFile* working_file =
        working_files->GetFileByFilename(update.value.path);
    if (working_file) {
      // Update indexed content.
      working_file->SetIndexContent(update.file_content);

      // Inactive lines.
      EmitInactiveLines(working_file, update.value.inactive_regions);

      // Semantic highlighting.
      int file_id =
          db->name2file_id[LowerPathIfInsensitive(working_file->filename)];
      QueryFile* file = &db->files[file_id];
      EmitSemanticHighlighting(db, semantic_cache, working_file, file);
    }
  }
}

}  // namespace

bool QueryDb_ImportMain(QueryDatabase* db,
                        ImportPipelineStatus* status,
                        SemanticHighlightSymbolCache* semantic_cache,
                        WorkingFiles* working_files) {
  auto* queue = QueueManager::instance();

  ActiveThread active_thread(status);

  bool did_work = false;

  for (int i = 80; i--; ) {
    std::optional<Index_OnIndexed> response = queue->on_indexed.TryPopFront();
    if (!response)
      break;
    did_work = true;
    QueryDb_OnIndexed(queue, db, status, semantic_cache, working_files,
                      &*response);
  }

  return did_work;
}
