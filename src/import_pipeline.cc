#include "import_pipeline.h"

#include "cache_manager.h"
#include "config.h"
#include "diagnostics_engine.h"
#include "iindexer.h"
#include "import_manager.h"
#include "lsp.h"
#include "message_handler.h"
#include "platform.h"
#include "project.h"
#include "query_utils.h"
#include "queue_manager.h"
#include "timer.h"
#include "timestamp_manager.h"

#include <doctest/doctest.h>
#include <loguru.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <vector>

namespace {

struct Out_Progress : public lsOutMessage<Out_Progress> {
  struct Params {
    int indexRequestCount = 0;
    int doIdMapCount = 0;
    int loadPreviousIndexCount = 0;
    int onIdMappedCount = 0;
    int onIndexedCount = 0;
    int activeThreads = 0;
  };
  std::string method = "$cquery/progress";
  Params params;
};
MAKE_REFLECT_STRUCT(Out_Progress::Params,
                    indexRequestCount,
                    doIdMapCount,
                    loadPreviousIndexCount,
                    onIdMappedCount,
                    onIndexedCount,
                    activeThreads);
MAKE_REFLECT_STRUCT(Out_Progress, jsonrpc, method, params);

// Instead of processing messages forever, we only process upto
// |kIterationSize| messages of a type at one time. While the import time
// likely stays the same, this should reduce overall queue lengths which means
// the user gets a usable index faster.
struct IterationLoop {
  const int kIterationSize = 100;
  int count = 0;

  bool Next() {
    return count++ < kIterationSize;
  }
  void Reset() {
    count = 0;
  }
};

struct IModificationTimestampFetcher {
  virtual ~IModificationTimestampFetcher() = default;
  virtual optional<int64_t> GetModificationTime(const std::string& path) = 0;
};
struct RealModificationTimestampFetcher : IModificationTimestampFetcher {
  ~RealModificationTimestampFetcher() override = default;

  // IModificationTimestamp:
  optional<int64_t> GetModificationTime(const std::string& path) override {
    return GetLastModificationTime(path);
  }
};
struct FakeModificationTimestampFetcher : IModificationTimestampFetcher {
  std::unordered_map<std::string, optional<int64_t>> entries;

  ~FakeModificationTimestampFetcher() override = default;

  // IModificationTimestamp:
  optional<int64_t> GetModificationTime(const std::string& path) override {
    auto it = entries.find(path);
    assert(it != entries.end());
    return it->second;
  }
};

long long GetCurrentTimeInMilliseconds() {
  auto time_since_epoch = Timer::Clock::now().time_since_epoch();
  long long elapsed_milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch)
          .count();
  return elapsed_milliseconds;
}

struct ActiveThread {
  ActiveThread(Config* config, ImportPipelineStatus* status)
      : config_(config), status_(status) {
    if (config_->progressReportFrequencyMs < 0)
      return;

    ++status_->num_active_threads;
  }
  ~ActiveThread() {
    if (config_->progressReportFrequencyMs < 0)
      return;

    --status_->num_active_threads;
    EmitProgress();
  }

  // Send indexing progress to client if reporting is enabled.
  void EmitProgress() {
    auto* queue = QueueManager::instance();
    Out_Progress out;
    out.params.indexRequestCount = queue->index_request.Size();
    out.params.doIdMapCount = queue->do_id_map.Size();
    out.params.loadPreviousIndexCount = queue->load_previous_index.Size();
    out.params.onIdMappedCount = queue->on_id_mapped.Size();
    out.params.onIndexedCount = queue->on_indexed.Size();
    out.params.activeThreads = status_->num_active_threads;

    // Ignore this progress update if the last update was too recent.
    if (config_->progressReportFrequencyMs != 0) {
      // Make sure we output a status update if queue lengths are zero.
      bool all_zero =
          out.params.indexRequestCount == 0 && out.params.doIdMapCount == 0 &&
          out.params.loadPreviousIndexCount == 0 &&
          out.params.onIdMappedCount == 0 && out.params.onIndexedCount == 0 &&
          out.params.activeThreads == 0;
      if (!all_zero &&
          GetCurrentTimeInMilliseconds() < status_->next_progress_output)
        return;
      status_->next_progress_output =
          GetCurrentTimeInMilliseconds() + config_->progressReportFrequencyMs;
    }

    QueueManager::WriteStdout(kMethodType_Unknown, out);
  }

  Config* config_;
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
    IModificationTimestampFetcher* modification_timestamp_fetcher,
    ImportManager* import_manager,
    const std::shared_ptr<ICacheManager>& cache_manager,
    IndexFile* opt_previous_index,
    const std::string& path,
    const std::vector<std::string>& args,
    const optional<std::string>& from) {
  auto unwrap_opt = [](const optional<std::string>& opt) -> std::string {
    if (opt)
      return " (via " + *opt + ")";
    return "";
  };

  // If the file is a dependency but another file as already imported it,
  // don't bother.
  if (!is_interactive && from &&
      !import_manager->TryMarkDependencyImported(path)) {
    return ShouldParse::No;
  }

  optional<int64_t> modification_timestamp =
      modification_timestamp_fetcher->GetModificationTime(path);

  // Cannot find file.
  if (!modification_timestamp)
    return ShouldParse::NoSuchFile;

  optional<int64_t> last_cached_modification =
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
    IModificationTimestampFetcher* modification_timestamp_fetcher,
    ImportManager* import_manager,
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
  ShouldParse path_state = FileNeedsParse(
      is_interactive, timestamp_manager, modification_timestamp_fetcher,
      import_manager, cache_manager, previous_index, path_to_index, entry.args,
      nullopt);
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

    if (FileNeedsParse(is_interactive, timestamp_manager,
                       modification_timestamp_fetcher, import_manager,
                       cache_manager, previous_index, dependency, entry.args,
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
  LOG_S(INFO) << "Skipping parse; no timestamp change for " << path_to_index;

  // TODO/FIXME: real perf
  PerformanceImportFile perf;

  std::vector<Index_DoIdMap> result;
  result.push_back(Index_DoIdMap(cache_manager->TakeOrLoad(path_to_index),
                                 cache_manager, perf, is_interactive,
                                 false /*write_to_disk*/));
  for (const std::string& dependency : previous_index->dependencies) {
    // Only load a dependency if it is not already loaded.
    //
    // This is important for perf in large projects where there are lots of
    // dependencies shared between many files.
    if (!file_consumer_shared->Mark(dependency))
      continue;

    LOG_S(INFO) << "Emitting index result for " << dependency << " (via "
                << previous_index->path << ")";

    std::unique_ptr<IndexFile> dependency_index =
        cache_manager->TryTakeOrLoad(dependency);

    // |dependency_index| may be null if there is no cache for it but
    // another file has already started importing it.
    if (!dependency_index)
      continue;

    result.push_back(Index_DoIdMap(std::move(dependency_index), cache_manager,
                                   perf, is_interactive,
                                   false /*write_to_disk*/));
  }

  QueueManager::instance()->do_id_map.EnqueueAll(std::move(result));
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
    optional<int64_t> mod_time = GetLastModificationTime(path);
    if (!mod_time)
      return "";

    if (*mod_time == cached_time)
      return cached;

    optional<std::string> fresh_content = ReadContent(path);
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

void ParseFile(Config* config,
               DiagnosticsEngine* diag_engine,
               WorkingFiles* working_files,
               FileConsumerSharedState* file_consumer_shared,
               TimestampManager* timestamp_manager,
               IModificationTimestampFetcher* modification_timestamp_fetcher,
               ImportManager* import_manager,
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
                       modification_timestamp_fetcher, import_manager,
                       request.cache_manager, request.is_interactive, entry,
                       path_to_index) == CacheLoadResult::DoNotParse) {
    return;
  }

  LOG_S(INFO) << "Parsing " << path_to_index;
  std::vector<FileContents> file_contents = PreloadFileContents(
      request.cache_manager, entry, request.contents, path_to_index);

  std::vector<Index_DoIdMap> result;
  PerformanceImportFile perf;
  auto indexes = indexer->Index(config, file_consumer_shared, path_to_index,
                                entry.args, file_contents, &perf);

  if (!indexes) {
    if (config->index.enabled &&
        !std::holds_alternative<std::monostate>(request.id)) {
      Out_Error out;
      out.id = request.id;
      out.error.code = lsErrorCodes::InternalError;
      out.error.message = "Failed to index " + path_to_index;
      QueueManager::WriteStdout(kMethodType_Unknown, out);
    }
    return;
  }

  for (std::unique_ptr<IndexFile>& new_index : *indexes) {
    Timer time;

    // Only emit diagnostics for non-interactive sessions, which makes it easier
    // to identify indexing problems. For interactive sessions, diagnostics are
    // handled by code completion.
    if (!request.is_interactive)
      diag_engine->Publish(working_files, new_index->path,
                           new_index->diagnostics_);

    // When main thread does IdMap request it will request the previous index if
    // needed.
    LOG_S(INFO) << "Emitting index result for " << new_index->path;
    result.push_back(Index_DoIdMap(std::move(new_index), request.cache_manager,
                                   perf, request.is_interactive,
                                   true /*write_to_disk*/));
  }

  QueueManager::instance()->do_id_map.EnqueueAll(std::move(result),
                                                 request.is_interactive);
}

bool IndexMain_DoParse(
    Config* config,
    DiagnosticsEngine* diag_engine,
    WorkingFiles* working_files,
    FileConsumerSharedState* file_consumer_shared,
    TimestampManager* timestamp_manager,
    IModificationTimestampFetcher* modification_timestamp_fetcher,
    ImportManager* import_manager,
    IIndexer* indexer) {
  auto* queue = QueueManager::instance();
  optional<Index_Request> request = queue->index_request.TryPopFront();
  if (!request)
    return false;

  Project::Entry entry;
  entry.filename = request->path;
  entry.args = request->args;
  ParseFile(config, diag_engine, working_files, file_consumer_shared,
            timestamp_manager, modification_timestamp_fetcher, import_manager,
            indexer, request.value(), entry);
  return true;
}

bool IndexMain_DoCreateIndexUpdate(TimestampManager* timestamp_manager) {
  auto* queue = QueueManager::instance();

  bool did_work = false;
  IterationLoop loop;
  while (loop.Next()) {
    optional<Index_OnIdMapped> response = queue->on_id_mapped.TryPopFront();
    if (!response)
      return did_work;

    did_work = true;

    Timer time;

    IdMap* previous_id_map = nullptr;
    IndexFile* previous_index = nullptr;
    if (response->previous) {
      previous_id_map = response->previous->ids.get();
      previous_index = response->previous->file.get();
    }

    // Build delta update.
    IndexUpdate update =
        IndexUpdate::CreateDelta(previous_id_map, response->current->ids.get(),
                                 previous_index, response->current->file.get());
    response->perf.index_make_delta = time.ElapsedMicrosecondsAndReset();
    LOG_S(INFO) << "Built index update for " << response->current->file->path
                << " (is_delta=" << !!response->previous << ")";

    // Write current index to disk if requested.
    if (response->write_to_disk) {
      LOG_S(INFO) << "Writing cached index to disk for "
                  << response->current->file->path;
      time.Reset();
      response->cache_manager->WriteToCache(*response->current->file);
      response->perf.index_save_to_disk = time.ElapsedMicrosecondsAndReset();
      timestamp_manager->UpdateCachedModificationTime(
          response->current->file->path,
          response->current->file->last_modification_time);
    }

  #if false
  #define PRINT_SECTION(name)                                                    \
    if (response->perf.name) {                                                   \
      total += response->perf.name;                                              \
      output << " " << #name << ": " << FormatMicroseconds(response->perf.name); \
    }
    std::stringstream output;
    long long total = 0;
    output << "[perf]";
    PRINT_SECTION(index_parse);
    PRINT_SECTION(index_build);
    PRINT_SECTION(index_save_to_disk);
    PRINT_SECTION(index_load_cached);
    PRINT_SECTION(querydb_id_map);
    PRINT_SECTION(index_make_delta);
    output << "\n       total: " << FormatMicroseconds(total);
    output << " path: " << response->current_index->path;
    LOG_S(INFO) << output.rdbuf();
  #undef PRINT_SECTION

    if (response->is_interactive)
      LOG_S(INFO) << "Applying IndexUpdate" << std::endl << update.ToString();
  #endif

    Index_OnIndexed reply(std::move(update), response->perf);
    queue->on_indexed.PushBack(std::move(reply), response->is_interactive);
  }

  return did_work;
}

bool IndexMain_LoadPreviousIndex() {
  auto* queue = QueueManager::instance();
  optional<Index_DoIdMap> response = queue->load_previous_index.TryPopFront();
  if (!response)
    return false;

  response->previous =
      response->cache_manager->TryTakeOrLoad(response->current->path);
  LOG_IF_S(ERROR, !response->previous)
      << "Unable to load previous index for already imported index "
      << response->current->path;

  queue->do_id_map.PushBack(std::move(*response));
  return true;
}

bool IndexMergeIndexUpdates() {
  auto* queue = QueueManager::instance();
  optional<Index_OnIndexed> root = queue->on_indexed.TryPopBack();
  if (!root)
    return false;

  bool did_merge = false;
  IterationLoop loop;
  while (loop.Next()) {
    optional<Index_OnIndexed> to_join = queue->on_indexed.TryPopBack();
    if (!to_join) {
      queue->on_indexed.PushFront(std::move(*root));
      return did_merge;
    }

    did_merge = true;
    Timer time;
    root->update.Merge(std::move(to_join->update));
    // time.ResetAndPrint("Joined querydb updates for files: " +
    // StringJoinMap(root->update.files_def_update,
    //[](const QueryFile::DefUpdate& update) {
    // return update.path;
    //}));
  }

  return did_merge;
}

}  // namespace

ImportPipelineStatus::ImportPipelineStatus()
    : num_active_threads(0), next_progress_output(0) {}

// Index a file using an already-parsed translation unit from code completion.
// Since most of the time for indexing a file comes from parsing, we can do
// real-time indexing.
// TODO: add option to disable this.
void IndexWithTuFromCodeCompletion(
    Config* config,
    FileConsumerSharedState* file_consumer_shared,
    ClangTranslationUnit* tu,
    const std::vector<CXUnsavedFile>& file_contents,
    const std::string& path,
    const std::vector<std::string>& args) {
  file_consumer_shared->Reset(path);

  PerformanceImportFile perf;
  ClangIndex index;
  auto indexes = ParseWithTu(config, file_consumer_shared, &perf, tu, &index,
                             path, args, file_contents);
  if (!indexes)
    return;

  std::vector<Index_DoIdMap> result;
  for (std::unique_ptr<IndexFile>& new_index : *indexes) {
    Timer time;

    std::shared_ptr<ICacheManager> cache_manager;
    assert(false && "FIXME cache_manager");
    // When main thread does IdMap request it will request the previous index if
    // needed.
    LOG_S(INFO) << "Emitting index result for " << new_index->path;
    result.push_back(Index_DoIdMap(std::move(new_index), cache_manager, perf,
                                   true /*is_interactive*/,
                                   true /*write_to_disk*/));
  }

  LOG_IF_S(WARNING, result.size() > 1)
      << "Code completion index update generated more than one index";

  QueueManager::instance()->do_id_map.EnqueueAll(std::move(result));
}

void Indexer_Main(Config* config,
                  DiagnosticsEngine* diag_engine,
                  FileConsumerSharedState* file_consumer_shared,
                  TimestampManager* timestamp_manager,
                  ImportManager* import_manager,
                  ImportPipelineStatus* status,
                  Project* project,
                  WorkingFiles* working_files,
                  MultiQueueWaiter* waiter) {
  RealModificationTimestampFetcher modification_timestamp_fetcher;
  auto* queue = QueueManager::instance();
  // Build one index per-indexer, as building the index acquires a global lock.
  auto indexer = IIndexer::MakeClangIndexer();

  while (true) {
    bool did_work = false;

    {
      ActiveThread active_thread(config, status);

      // TODO: process all off IndexMain_DoIndex before calling
      // IndexMain_DoCreateIndexUpdate for better icache behavior. We need to
      // have some threads spinning on both though otherwise memory usage will
      // get bad.

      // We need to make sure to run both IndexMain_DoParse and
      // IndexMain_DoCreateIndexUpdate so we don't starve querydb from doing any
      // work. Running both also lets the user query the partially constructed
      // index.
      did_work = IndexMain_DoParse(config, diag_engine, working_files,
                                   file_consumer_shared, timestamp_manager,
                                   &modification_timestamp_fetcher,
                                   import_manager, indexer.get()) ||
                 did_work;

      did_work = IndexMain_DoCreateIndexUpdate(timestamp_manager) || did_work;

      did_work = IndexMain_LoadPreviousIndex() || did_work;

      // Nothing to index and no index updates to create, so join some already
      // created index updates to reduce work on querydb thread.
      if (!did_work)
        did_work = IndexMergeIndexUpdates() || did_work;
    }

    // We didn't do any work, so wait for a notification.
    if (!did_work) {
      waiter->Wait(&queue->on_indexed, &queue->index_request,
                   &queue->on_id_mapped, &queue->load_previous_index);
    }
  }
}

namespace {
void QueryDb_DoIdMap(QueueManager* queue,
                     QueryDatabase* db,
                     ImportManager* import_manager,
                     Index_DoIdMap* request) {
  assert(request->current);

  // If the request does not have previous state and we have already imported
  // it, load the previous state from disk and rerun IdMap logic later. Do not
  // do this if we have already attempted in the past.
  if (!request->load_previous && !request->previous &&
      db->usr_to_file.find(NormalizedPath(request->current->path)) !=
          db->usr_to_file.end()) {
    assert(!request->load_previous);
    request->load_previous = true;
    queue->load_previous_index.PushBack(std::move(*request));
    return;
  }

  // Check if the file is already being imported into querydb. If it is, drop
  // the request.
  //
  // Note, we must do this *after* we have checked for the previous index,
  // otherwise we will never actually generate the IdMap.
  if (!import_manager->StartQueryDbImport(request->current->path)) {
    LOG_S(INFO) << "Dropping index as it is already being imported for "
                << request->current->path;
    return;
  }

  Index_OnIdMapped response(request->cache_manager, request->perf,
                            request->is_interactive, request->write_to_disk);
  Timer time;

  auto make_map = [db](std::unique_ptr<IndexFile> file)
      -> std::unique_ptr<Index_OnIdMapped::File> {
    if (!file)
      return nullptr;

    auto id_map = std::make_unique<IdMap>(db, file->id_cache);
    return std::make_unique<Index_OnIdMapped::File>(std::move(file),
                                                    std::move(id_map));
  };
  response.current = make_map(std::move(request->current));
  response.previous = make_map(std::move(request->previous));
  response.perf.querydb_id_map = time.ElapsedMicrosecondsAndReset();

  queue->on_id_mapped.PushBack(std::move(response));
}

void QueryDb_OnIndexed(QueueManager* queue,
                       QueryDatabase* db,
                       ImportManager* import_manager,
                       ImportPipelineStatus* status,
                       SemanticHighlightSymbolCache* semantic_cache,
                       WorkingFiles* working_files,
                       Index_OnIndexed* response) {
  Timer time;
  db->ApplyIndexUpdate(&response->update);
  time.ResetAndPrint("Applying index update for " +
                     StringJoinMap(response->update.files_def_update,
                                   [](const QueryFile::DefUpdate& value) {
                                     return value.value.path;
                                   }));

  // Update indexed content, inactive lines, and semantic highlighting.
  for (auto& updated_file : response->update.files_def_update) {
    WorkingFile* working_file =
        working_files->GetFileByFilename(updated_file.value.path);
    if (working_file) {
      // Update indexed content.
      working_file->SetIndexContent(updated_file.file_content);

      // Inactive lines.
      EmitInactiveLines(working_file, updated_file.value.inactive_regions);

      // Semantic highlighting.
      QueryFileId file_id =
          db->usr_to_file[NormalizedPath(working_file->filename)];
      QueryFile* file = &db->files[file_id.id];
      EmitSemanticHighlighting(db, semantic_cache, working_file, file);
    }

    // Mark the files as being done in querydb stage after we apply the index
    // update.
    import_manager->DoneQueryDbImport(updated_file.value.path);
  }
}

}  // namespace

bool QueryDb_ImportMain(Config* config,
                        QueryDatabase* db,
                        ImportManager* import_manager,
                        ImportPipelineStatus* status,
                        SemanticHighlightSymbolCache* semantic_cache,
                        WorkingFiles* working_files) {
  auto* queue = QueueManager::instance();

  ActiveThread active_thread(config, status);

  bool did_work = false;

  IterationLoop loop;
  while (loop.Next()) {
    optional<Index_DoIdMap> request = queue->do_id_map.TryPopFront();
    if (!request)
      break;
    did_work = true;
    QueryDb_DoIdMap(queue, db, import_manager, &*request);
  }

  loop.Reset();
  while (loop.Next()) {
    optional<Index_OnIndexed> response = queue->on_indexed.TryPopFront();
    if (!response)
      break;
    did_work = true;
    QueryDb_OnIndexed(queue, db, import_manager, status, semantic_cache,
                      working_files, &*response);
  }

  return did_work;
}

TEST_SUITE("ImportPipeline") {
  struct Fixture {
    Fixture() {
      QueueManager::Init(&querydb_waiter, &indexer_waiter, &stdout_waiter);

      queue = QueueManager::instance();
      cache_manager = ICacheManager::MakeFake({});
      indexer = IIndexer::MakeTestIndexer({});
      diag_engine.Init(&config);
    }

    bool PumpOnce() {
      return IndexMain_DoParse(&config, &diag_engine, &working_files,
                               &file_consumer_shared, &timestamp_manager,
                               &modification_timestamp_fetcher, &import_manager,
                               indexer.get());
    }

    void MakeRequest(const std::string& path,
                     const std::vector<std::string>& args = {},
                     bool is_interactive = false,
                     const std::string& contents = "void foo();") {
      queue->index_request.PushBack(
          Index_Request(path, args, is_interactive, contents, cache_manager));
    }

    MultiQueueWaiter querydb_waiter;
    MultiQueueWaiter indexer_waiter;
    MultiQueueWaiter stdout_waiter;

    QueueManager* queue = nullptr;
    Config config;
    DiagnosticsEngine diag_engine;
    WorkingFiles working_files;
    FileConsumerSharedState file_consumer_shared;
    TimestampManager timestamp_manager;
    FakeModificationTimestampFetcher modification_timestamp_fetcher;
    ImportManager import_manager;
    std::shared_ptr<ICacheManager> cache_manager;
    std::unique_ptr<IIndexer> indexer;
  };

  TEST_CASE_FIXTURE(Fixture, "FileNeedsParse") {
    auto check = [&](const std::string& file, bool is_dependency = false,
                     bool is_interactive = false,
                     const std::vector<std::string>& old_args = {},
                     const std::vector<std::string>& new_args = {}) {
      std::unique_ptr<IndexFile> opt_previous_index;
      if (!old_args.empty()) {
        opt_previous_index = std::make_unique<IndexFile>("---.cc", "<empty>");
        opt_previous_index->args = old_args;
      }
      optional<std::string> from;
      if (is_dependency)
        from = std::string("---.cc");
      return FileNeedsParse(is_interactive /*is_interactive*/,
                            &timestamp_manager, &modification_timestamp_fetcher,
                            &import_manager, cache_manager,
                            opt_previous_index.get(), file, new_args, from);
    };

    // A file with no timestamp is not imported, since this implies the file no
    // longer exists on disk.
    modification_timestamp_fetcher.entries["bar.h"] = nullopt;
    REQUIRE(check("bar.h", false /*is_dependency*/) == ShouldParse::NoSuchFile);

    // A dependency is only imported once.
    modification_timestamp_fetcher.entries["foo.h"] = 5;
    REQUIRE(check("foo.h", true /*is_dependency*/) == ShouldParse::Yes);
    REQUIRE(check("foo.h", true /*is_dependency*/) == ShouldParse::No);

    // An interactive dependency is imported.
    REQUIRE(check("foo.h", true /*is_dependency*/) == ShouldParse::No);
    REQUIRE(check("foo.h", true /*is_dependency*/, true /*is_interactive*/) ==
            ShouldParse::Yes);

    // A file whose timestamp has not changed is not imported. When the
    // timestamp changes (either forward or backward) it is reimported.
    auto check_timestamp_change = [&](int64_t timestamp) {
      modification_timestamp_fetcher.entries["aa.cc"] = timestamp;
      REQUIRE(check("aa.cc") == ShouldParse::Yes);
      REQUIRE(check("aa.cc") == ShouldParse::Yes);
      REQUIRE(check("aa.cc") == ShouldParse::Yes);
      timestamp_manager.UpdateCachedModificationTime("aa.cc", timestamp);
      REQUIRE(check("aa.cc") == ShouldParse::No);
    };
    check_timestamp_change(5);
    check_timestamp_change(6);
    check_timestamp_change(5);
    check_timestamp_change(4);

    // Argument change implies reimport, even if timestamp has not changed.
    timestamp_manager.UpdateCachedModificationTime("aa.cc", 5);
    modification_timestamp_fetcher.entries["aa.cc"] = 5;
    REQUIRE(check("aa.cc", false /*is_dependency*/, false /*is_interactive*/,
                  {"b"} /*old_args*/,
                  {"b", "a"} /*new_args*/) == ShouldParse::Yes);
  }

  // FIXME: validate other state like timestamp_manager, etc.
  // FIXME: add more interesting tests that are not the happy path
  // FIXME: test
  //   - IndexMain_DoCreateIndexUpdate
  //   - IndexMain_LoadPreviousIndex
  //   - QueryDb_ImportMain

  TEST_CASE_FIXTURE(Fixture, "index request with zero results") {
    indexer = IIndexer::MakeTestIndexer({IIndexer::TestEntry{"foo.cc", 0}});

    MakeRequest("foo.cc");

    REQUIRE(queue->index_request.Size() == 1);
    REQUIRE(queue->do_id_map.Size() == 0);
    PumpOnce();
    REQUIRE(queue->index_request.Size() == 0);
    REQUIRE(queue->do_id_map.Size() == 0);

    REQUIRE(file_consumer_shared.used_files.empty());
  }

  TEST_CASE_FIXTURE(Fixture, "one index request") {
    indexer = IIndexer::MakeTestIndexer({IIndexer::TestEntry{"foo.cc", 100}});

    MakeRequest("foo.cc");

    REQUIRE(queue->index_request.Size() == 1);
    REQUIRE(queue->do_id_map.Size() == 0);
    PumpOnce();
    REQUIRE(queue->index_request.Size() == 0);
    REQUIRE(queue->do_id_map.Size() == 100);

    REQUIRE(file_consumer_shared.used_files.empty());
  }

  TEST_CASE_FIXTURE(Fixture, "multiple index requests") {
    indexer = IIndexer::MakeTestIndexer(
        {IIndexer::TestEntry{"foo.cc", 100}, IIndexer::TestEntry{"bar.cc", 5}});

    MakeRequest("foo.cc");
    MakeRequest("bar.cc");

    REQUIRE(queue->index_request.Size() == 2);
    REQUIRE(queue->do_id_map.Size() == 0);
    while (PumpOnce()) {
    }
    REQUIRE(queue->index_request.Size() == 0);
    REQUIRE(queue->do_id_map.Size() == 105);

    REQUIRE(file_consumer_shared.used_files.empty());
  }
}
