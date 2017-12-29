#include "import_pipeline.h"

// TODO: cleanup includes
#include "cache.h"
#include "cache_loader.h"
#include "clang_complete.h"
#include "file_consumer.h"
#include "include_complete.h"
#include "indexer.h"
#include "language_server_api.h"
#include "lex_utils.h"
#include "lru_cache.h"
#include "match.h"
#include "message_handler.h"
#include "options.h"
#include "platform.h"
#include "project.h"
#include "query.h"
#include "query_utils.h"
#include "queue_manager.h"
#include "serializer.h"
#include "standard_includes.h"
#include "test.h"
#include "threaded_queue.h"
#include "timer.h"
#include "timestamp_manager.h"
#include "work_thread.h"
#include "working_files.h"

#include <doctest/doctest.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <loguru.hpp>

#include <climits>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

ImportPipelineStatus::ImportPipelineStatus() : num_active_threads(0) {}

// Send indexing progress to client if reporting is enabled.
void EmitProgress(Config* config) {
  if (config->enableProgressReports) {
    auto* queue = QueueManager::instance();
    Out_Progress out;
    out.params.indexRequestCount = queue->index_request.Size();
    out.params.doIdMapCount = queue->do_id_map.Size();
    out.params.loadPreviousIndexCount = queue->load_previous_index.Size();
    out.params.onIdMappedCount = queue->on_id_mapped.Size();
    out.params.onIndexedCount = queue->on_indexed.Size();

    QueueManager::WriteStdout(IpcId::Unknown, out);
  }
}

enum class FileParseQuery { NeedsParse, DoesNotNeedParse, NoSuchFile };

std::vector<Index_DoIdMap> DoParseFile(
    Config* config,
    WorkingFiles* working_files,
    ClangIndex* index,
    FileConsumer::SharedState* file_consumer_shared,
    TimestampManager* timestamp_manager,
    ImportManager* import_manager,
    CacheLoader* cache_loader,
    bool is_interactive,
    const std::string& path,
    const std::vector<std::string>& args,
    const optional<FileContents>& contents) {
  std::vector<Index_DoIdMap> result;

  // Always run this block, even if we are interactive, so we can check
  // dependencies and reset files in |file_consumer_shared|.
  IndexFile* previous_index = cache_loader->TryLoad(path);
  if (previous_index) {
    // If none of the dependencies have changed and the index is not
    // interactive (ie, requested by a file save), skip parsing and just load
    // from cache.

    // Checks if |path| needs to be reparsed. This will modify cached state
    // such that calling this function twice with the same path may return true
    // the first time but will return false the second.
    auto file_needs_parse = [&](const std::string& path, bool is_dependency) {
      // If the file is a dependency but another file as already imported it,
      // don't bother.
      if (!is_interactive && is_dependency &&
          !import_manager->TryMarkDependencyImported(path)) {
        return FileParseQuery::DoesNotNeedParse;
      }

      optional<int64_t> modification_timestamp = GetLastModificationTime(path);

      // Cannot find file.
      if (!modification_timestamp)
        return FileParseQuery::NoSuchFile;

      optional<int64_t> last_cached_modification =
          timestamp_manager->GetLastCachedModificationTime(cache_loader, path);

      // File has been changed.
      if (!last_cached_modification ||
          modification_timestamp != *last_cached_modification) {
        file_consumer_shared->Reset(path);
        timestamp_manager->UpdateCachedModificationTime(
            path, *modification_timestamp);
        return FileParseQuery::NeedsParse;
      }

      // File has not changed, do not parse it.
      return FileParseQuery::DoesNotNeedParse;
    };

    // Check timestamps and update |file_consumer_shared|.
    FileParseQuery path_state = file_needs_parse(path, false /*is_dependency*/);

    // Target file does not exist on disk, do not emit any indexes.
    // TODO: Dependencies should be reassigned to other files. We can do this by
    // updating the "primary_file" if it doesn't exist. Might not actually be a
    // problem in practice.
    if (path_state == FileParseQuery::NoSuchFile)
      return result;

    bool needs_reparse =
        is_interactive || path_state == FileParseQuery::NeedsParse;

    for (const std::string& dependency : previous_index->dependencies) {
      assert(!dependency.empty());

      // note: Use != as there are multiple failure results for FileParseQuery.
      if (file_needs_parse(dependency, true /*is_dependency*/) !=
          FileParseQuery::DoesNotNeedParse) {
        LOG_S(INFO) << "Timestamp has changed for " << dependency << " (via "
                    << previous_index->path << ")";
        needs_reparse = true;
        // SUBTLE: Do not break here, as |file_consumer_shared| is updated
        // inside of |file_needs_parse|.
      }
    }

    // No timestamps changed - load directly from cache.
    if (!needs_reparse) {
      LOG_S(INFO) << "Skipping parse; no timestamp change for " << path;

      // TODO/FIXME: real perf
      PerformanceImportFile perf;
      result.push_back(Index_DoIdMap(cache_loader->TakeOrLoad(path), perf,
                                     is_interactive, false /*write_to_disk*/));
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
            cache_loader->TryTakeOrLoad(dependency);

        // |dependency_index| may be null if there is no cache for it but
        // another file has already started importing it.
        if (!dependency_index)
          continue;

        result.push_back(Index_DoIdMap(std::move(dependency_index), perf,
                                       is_interactive,
                                       false /*write_to_disk*/));
      }
      return result;
    }
  }

  LOG_S(INFO) << "Parsing " << path;

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
  bool loaded_primary = false;
  std::vector<FileContents> file_contents;
  if (contents) {
    loaded_primary = loaded_primary || contents->path == path;
    file_contents.push_back(*contents);
  }
  for (const auto& it : cache_loader->caches) {
    const std::unique_ptr<IndexFile>& index = it.second;
    assert(index);
    optional<std::string> index_content = ReadContent(index->path);
    if (!index_content) {
      LOG_S(ERROR) << "Failed to preload index content for " << index->path;
      continue;
    }
    file_contents.push_back(FileContents(index->path, *index_content));

    loaded_primary = loaded_primary || index->path == path;
  }
  if (!loaded_primary) {
    optional<std::string> content = ReadContent(path);
    if (!content) {
      LOG_S(ERROR) << "Skipping index (file cannot be found): " << path;
      return result;
    }
    file_contents.push_back(FileContents(path, *content));
  }

  PerformanceImportFile perf;
  std::vector<std::unique_ptr<IndexFile>> indexes = Parse(
      config, file_consumer_shared, path, args, file_contents, &perf, index);
  for (std::unique_ptr<IndexFile>& new_index : indexes) {
    Timer time;

    // Only emit diagnostics for non-interactive sessions, which makes it easier
    // to identify indexing problems. For interactive sessions, diagnostics are
    // handled by code completion.
    if (!is_interactive)
      EmitDiagnostics(working_files, new_index->path, new_index->diagnostics_);

    // When main thread does IdMap request it will request the previous index if
    // needed.
    LOG_S(INFO) << "Emitting index result for " << new_index->path;
    result.push_back(Index_DoIdMap(std::move(new_index), perf, is_interactive,
                                   true /*write_to_disk*/));
  }

  return result;
}

// Index a file using an already-parsed translation unit from code completion.
// Since most of the time for indexing a file comes from parsing, we can do
// real-time indexing.
// TODO: add option to disable this.
void IndexWithTuFromCodeCompletion(
    FileConsumer::SharedState* file_consumer_shared,
    ClangTranslationUnit* tu,
    const std::vector<CXUnsavedFile>& file_contents,
    const std::string& path,
    const std::vector<std::string>& args) {
  file_consumer_shared->Reset(path);

  PerformanceImportFile perf;
  ClangIndex index;
  std::vector<std::unique_ptr<IndexFile>> indexes = ParseWithTu(
      file_consumer_shared, &perf, tu, &index, path, args, file_contents);

  std::vector<Index_DoIdMap> result;
  for (std::unique_ptr<IndexFile>& new_index : indexes) {
    Timer time;

    // When main thread does IdMap request it will request the previous index if
    // needed.
    LOG_S(INFO) << "Emitting index result for " << new_index->path;
    result.push_back(Index_DoIdMap(std::move(new_index), perf,
                                   true /*is_interactive*/,
                                   true /*write_to_disk*/));
  }

  LOG_IF_S(WARNING, result.size() > 1)
      << "Code completion index update generated more than one index";

  QueueManager::instance()->do_id_map.EnqueueAll(std::move(result));
}

std::vector<Index_DoIdMap> ParseFile(
    Config* config,
    WorkingFiles* working_files,
    ClangIndex* index,
    FileConsumer::SharedState* file_consumer_shared,
    TimestampManager* timestamp_manager,
    ImportManager* import_manager,
    bool is_interactive,
    const Project::Entry& entry,
    const optional<std::string>& contents) {
  optional<FileContents> file_contents;
  if (contents)
    file_contents = FileContents(entry.filename, *contents);

  CacheLoader cache_loader(config);

  // Try to determine the original import file by loading the file from cache.
  // This lets the user request an index on a header file, which clang will
  // complain about if indexed by itself.
  IndexFile* entry_cache = cache_loader.TryLoad(entry.filename);
  std::string tu_path = entry_cache ? entry_cache->import_file : entry.filename;
  return DoParseFile(config, working_files, index, file_consumer_shared,
                     timestamp_manager, import_manager, &cache_loader,
                     is_interactive, tu_path, entry.args, file_contents);
}

bool IndexMain_DoParse(Config* config,
                       WorkingFiles* working_files,
                       FileConsumer::SharedState* file_consumer_shared,
                       TimestampManager* timestamp_manager,
                       ImportManager* import_manager,
                       ClangIndex* index) {
  auto* queue = QueueManager::instance();
  optional<Index_Request> request = queue->index_request.TryDequeue();
  if (!request)
    return false;

  Project::Entry entry;
  entry.filename = request->path;
  entry.args = request->args;
  std::vector<Index_DoIdMap> responses = ParseFile(
      config, working_files, index, file_consumer_shared, timestamp_manager,
      import_manager, request->is_interactive, entry, request->contents);

  // Don't bother sending an IdMap request if there are no responses.
  if (responses.empty())
    return false;

  // EnqueueAll will clear |responses|.
  queue->do_id_map.EnqueueAll(std::move(responses));
  return true;
}

bool IndexMain_DoCreateIndexUpdate(Config* config,
                                   TimestampManager* timestamp_manager) {
  auto* queue = QueueManager::instance();
  optional<Index_OnIdMapped> response = queue->on_id_mapped.TryDequeue();
  if (!response)
    return false;

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
    WriteToCache(config, *response->current->file);
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

  Index_OnIndexed reply(update, response->perf);
  queue->on_indexed.Enqueue(std::move(reply));

  return true;
}

bool IndexMain_LoadPreviousIndex(Config* config) {
  auto* queue = QueueManager::instance();
  optional<Index_DoIdMap> response = queue->load_previous_index.TryDequeue();
  if (!response)
    return false;

  response->previous = LoadCachedIndex(config, response->current->path);
  LOG_IF_S(ERROR, !response->previous)
      << "Unable to load previous index for already imported index "
      << response->current->path;

  queue->do_id_map.Enqueue(std::move(*response));
  return true;
}

bool IndexMergeIndexUpdates() {
  auto* queue = QueueManager::instance();
  optional<Index_OnIndexed> root = queue->on_indexed.TryDequeue();
  if (!root)
    return false;

  bool did_merge = false;
  while (true) {
    optional<Index_OnIndexed> to_join = queue->on_indexed.TryDequeue();
    if (!to_join) {
      queue->on_indexed.Enqueue(std::move(*root));
      return did_merge;
    }

    did_merge = true;
    Timer time;
    root->update.Merge(to_join->update);
    // time.ResetAndPrint("Joined querydb updates for files: " +
    // StringJoinMap(root->update.files_def_update,
    //[](const QueryFile::DefUpdate& update) {
    // return update.path;
    //}));
  }
}

void Indexer_Main(Config* config,
                  FileConsumer::SharedState* file_consumer_shared,
                  TimestampManager* timestamp_manager,
                  ImportManager* import_manager,
                  ImportPipelineStatus* status,
                  Project* project,
                  WorkingFiles* working_files,
                  MultiQueueWaiter* waiter) {
  auto* queue = QueueManager::instance();
  // Build one index per-indexer, as building the index acquires a global lock.
  ClangIndex index;

  while (true) {
    status->num_active_threads++;

    EmitProgress(config);

    // TODO: process all off IndexMain_DoIndex before calling
    // IndexMain_DoCreateIndexUpdate for better icache behavior. We need to have
    // some threads spinning on both though otherwise memory usage will get bad.

    // We need to make sure to run both IndexMain_DoParse and
    // IndexMain_DoCreateIndexUpdate so we don't starve querydb from doing any
    // work. Running both also lets the user query the partially constructed
    // index.
    bool did_parse =
        IndexMain_DoParse(config, working_files, file_consumer_shared,
                          timestamp_manager, import_manager, &index);

    bool did_create_update =
        IndexMain_DoCreateIndexUpdate(config, timestamp_manager);

    bool did_load_previous = IndexMain_LoadPreviousIndex(config);

    // Nothing to index and no index updates to create, so join some already
    // created index updates to reduce work on querydb thread.
    bool did_merge = false;
    if (!did_parse && !did_create_update && !did_load_previous)
      did_merge = IndexMergeIndexUpdates();

    status->num_active_threads--;

    // We didn't do any work, so wait for a notification.
    if (!did_parse && !did_create_update && !did_merge && !did_load_previous) {
      waiter->Wait({&queue->index_request, &queue->on_id_mapped,
                    &queue->load_previous_index, &queue->on_indexed});
    }
  }
}

bool QueryDb_ImportMain(Config* config,
                        QueryDatabase* db,
                        ImportManager* import_manager,
                        SemanticHighlightSymbolCache* semantic_cache,
                        WorkingFiles* working_files) {
  auto* queue = QueueManager::instance();
  EmitProgress(config);

  bool did_work = false;

  while (true) {
    optional<Index_DoIdMap> request = queue->do_id_map.TryDequeue();
    if (!request)
      break;
    did_work = true;

    assert(request->current);

    // If the request does not have previous state and we have already imported
    // it, load the previous state from disk and rerun IdMap logic later. Do not
    // do this if we have already attempted in the past.
    if (!request->load_previous && !request->previous &&
        db->usr_to_file.find(LowerPathIfCaseInsensitive(
            request->current->path)) != db->usr_to_file.end()) {
      assert(!request->load_previous);
      request->load_previous = true;
      queue->load_previous_index.Enqueue(std::move(*request));
      continue;
    }

    // Check if the file is already being imported into querydb. If it is, drop
    // the request.
    //
    // Note, we must do this *after* we have checked for the previous index,
    // otherwise we will never actually generate the IdMap.
    if (!import_manager->StartQueryDbImport(request->current->path)) {
      LOG_S(INFO) << "Dropping index as it is already being imported for "
                  << request->current->path;
      continue;
    }

    Index_OnIdMapped response(request->perf, request->is_interactive,
                              request->write_to_disk);
    Timer time;

    auto make_map = [db](std::unique_ptr<IndexFile> file)
        -> std::unique_ptr<Index_OnIdMapped::File> {
      if (!file)
        return nullptr;

      auto id_map = MakeUnique<IdMap>(db, file->id_cache);
      return MakeUnique<Index_OnIdMapped::File>(std::move(file),
                                                std::move(id_map));
    };
    response.current = make_map(std::move(request->current));
    response.previous = make_map(std::move(request->previous));
    response.perf.querydb_id_map = time.ElapsedMicrosecondsAndReset();

    queue->on_id_mapped.Enqueue(std::move(response));
  }

  while (true) {
    optional<Index_OnIndexed> response = queue->on_indexed.TryDequeue();
    if (!response)
      break;

    did_work = true;

    Timer time;

    for (auto& updated_file : response->update.files_def_update) {
      // TODO: We're reading a file on querydb thread. This is slow!! If this
      // a real problem in practice we can load the file in a previous stage.
      // It should be fine though because we only do it if the user has the
      // file open.
      WorkingFile* working_file =
          working_files->GetFileByFilename(updated_file.path);
      if (working_file) {
        optional<std::string> cached_file_contents =
            LoadCachedFileContents(config, updated_file.path);
        if (cached_file_contents)
          working_file->SetIndexContent(*cached_file_contents);
        else
          working_file->SetIndexContent(working_file->buffer_content);
        time.ResetAndPrint(
            "Update WorkingFile index contents (via disk load) for " +
            updated_file.path);

        // Update inactive region.
        EmitInactiveLines(working_file, updated_file.inactive_regions);
      }
    }

    time.Reset();
    db->ApplyIndexUpdate(&response->update);
    time.ResetAndPrint("Applying index update for " +
                       StringJoinMap(response->update.files_def_update,
                                     [](const QueryFile::DefUpdate& value) {
                                       return value.path;
                                     }));

    // Update semantic highlighting.
    for (auto& updated_file : response->update.files_def_update) {
      WorkingFile* working_file =
          working_files->GetFileByFilename(updated_file.path);
      if (working_file) {
        QueryFileId file_id =
            db->usr_to_file[LowerPathIfCaseInsensitive(working_file->filename)];
        QueryFile* file = &db->files[file_id.id];
        EmitSemanticHighlighting(db, semantic_cache, working_file, file);
      }
    }

    // Mark the files as being done in querydb stage after we apply the index
    // update.
    for (auto& updated_file : response->update.files_def_update)
      import_manager->DoneQueryDbImport(updated_file.path);
  }

  return did_work;
}
