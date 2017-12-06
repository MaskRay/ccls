// TODO: cleanup includes
#include "cache.h"
#include "cache_loader.h"
#include "clang_complete.h"
#include "file_consumer.h"
#include "include_complete.h"
#include "indexer.h"
#include "ipc_manager.h"
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

// TODO: provide a feature like 'https://github.com/goldsborough/clang-expand',
// ie, a fully linear view of a function with inline function calls expanded.
// We can probably use vscode decorators to achieve it.

// TODO: implement ThreadPool type which monitors CPU usage / number of work
// items per second completed and scales up/down number of running threads.

namespace {

std::vector<std::string> kEmptyArgs;

// If true stdout will be printed to stderr.
bool g_log_stdin_stdout_to_stderr = false;

// This function returns true if e2e timing should be displayed for the given
// IpcId.
bool ShouldDisplayIpcTiming(IpcId id) {
  switch (id) {
    case IpcId::TextDocumentPublishDiagnostics:
    case IpcId::CqueryPublishInactiveRegions:
    case IpcId::Unknown:
      return false;
    default:
      return true;
  }
}

void EmitDiagnostics(WorkingFiles* working_files,
                     std::string path,
                     NonElidedVector<lsDiagnostic> diagnostics) {
  // Emit diagnostics.
  Out_TextDocumentPublishDiagnostics out;
  out.params.uri = lsDocumentUri::FromPath(path);
  out.params.diagnostics = diagnostics;
  IpcManager::WriteStdout(IpcId::TextDocumentPublishDiagnostics, out);

  // Cache diagnostics so we can show fixits.
  working_files->DoActionOnFile(path, [&](WorkingFile* working_file) {
    if (working_file)
      working_file->diagnostics_ = diagnostics;
  });
}

REGISTER_IPC_MESSAGE(Ipc_CancelRequest);
REGISTER_IPC_MESSAGE(Ipc_InitializeRequest);
REGISTER_IPC_MESSAGE(Ipc_InitializedNotification);
REGISTER_IPC_MESSAGE(Ipc_Exit);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDidOpen);
REGISTER_IPC_MESSAGE(Ipc_CqueryTextDocumentDidView);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDidChange);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDidClose);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDidSave);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentRename);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentComplete);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentSignatureHelp);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDefinition);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDocumentHighlight);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentHover);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentReferences);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDocumentSymbol);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDocumentLink);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentCodeAction);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentCodeLens);
REGISTER_IPC_MESSAGE(Ipc_CodeLensResolve);
REGISTER_IPC_MESSAGE(Ipc_WorkspaceSymbol);
REGISTER_IPC_MESSAGE(Ipc_CqueryFreshenIndex);
REGISTER_IPC_MESSAGE(Ipc_CqueryTypeHierarchyTree);
REGISTER_IPC_MESSAGE(Ipc_CqueryCallTreeInitial);
REGISTER_IPC_MESSAGE(Ipc_CqueryCallTreeExpand);
REGISTER_IPC_MESSAGE(Ipc_CqueryVars);
REGISTER_IPC_MESSAGE(Ipc_CqueryCallers);
REGISTER_IPC_MESSAGE(Ipc_CqueryBase);
REGISTER_IPC_MESSAGE(Ipc_CqueryDerived);
REGISTER_IPC_MESSAGE(Ipc_CqueryIndexFile);
REGISTER_IPC_MESSAGE(Ipc_CqueryQueryDbWaitForIdleIndexer);
REGISTER_IPC_MESSAGE(Ipc_CqueryExitWhenIdle);

// Send indexing progress to client if reporting is enabled.
void EmitProgress(Config* config, QueueManager* queue) {
  if (config->enableProgressReports) {
    Out_Progress out;
    out.params.indexRequestCount = queue->index_request.Size();
    out.params.doIdMapCount = queue->do_id_map.Size();
    out.params.loadPreviousIndexCount = queue->load_previous_index.Size();
    out.params.onIdMappedCount = queue->on_id_mapped.Size();
    out.params.onIndexedCount = queue->on_indexed.Size();

    IpcManager::WriteStdout(IpcId::Unknown, out);
  }
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// IMPORT PIPELINE /////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

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
    QueueManager* queue,
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

  queue->do_id_map.EnqueueAll(std::move(result));
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
                       QueueManager* queue,
                       FileConsumer::SharedState* file_consumer_shared,
                       TimestampManager* timestamp_manager,
                       ImportManager* import_manager,
                       ClangIndex* index) {
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
                                   QueueManager* queue,
                                   TimestampManager* timestamp_manager) {
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

bool IndexMain_LoadPreviousIndex(Config* config, QueueManager* queue) {
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

bool IndexMergeIndexUpdates(QueueManager* queue) {
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

WorkThread::Result IndexMain(Config* config,
                             FileConsumer::SharedState* file_consumer_shared,
                             TimestampManager* timestamp_manager,
                             ImportManager* import_manager,
                             Project* project,
                             WorkingFiles* working_files,
                             MultiQueueWaiter* waiter,
                             QueueManager* queue) {
  EmitProgress(config, queue);

  // TODO: dispose of index after it is not used for a while.
  ClangIndex index;

  // TODO: process all off IndexMain_DoIndex before calling
  // IndexMain_DoCreateIndexUpdate for
  //       better icache behavior. We need to have some threads spinning on
  //       both though
  //       otherwise memory usage will get bad.

  // We need to make sure to run both IndexMain_DoParse and
  // IndexMain_DoCreateIndexUpdate so we don't starve querydb from doing any
  // work. Running both also lets the user query the partially constructed
  // index.
  bool did_parse =
      IndexMain_DoParse(config, working_files, queue, file_consumer_shared,
                        timestamp_manager, import_manager, &index);

  bool did_create_update =
      IndexMain_DoCreateIndexUpdate(config, queue, timestamp_manager);

  bool did_load_previous = IndexMain_LoadPreviousIndex(config, queue);

  // Nothing to index and no index updates to create, so join some already
  // created index updates to reduce work on querydb thread.
  bool did_merge = false;
  if (!did_parse && !did_create_update && !did_load_previous)
    did_merge = IndexMergeIndexUpdates(queue);

  // We didn't do any work, so wait for a notification.
  if (!did_parse && !did_create_update && !did_merge && !did_load_previous) {
    waiter->Wait({&queue->index_request, &queue->on_id_mapped,
                  &queue->load_previous_index, &queue->on_indexed});
  }

  return queue->HasWork() ? WorkThread::Result::MoreWork
                          : WorkThread::Result::NoWork;
}

bool QueryDb_ImportMain(Config* config,
                        QueryDatabase* db,
                        ImportManager* import_manager,
                        QueueManager* queue,
                        SemanticHighlightSymbolCache* semantic_cache,
                        WorkingFiles* working_files) {
  EmitProgress(config, queue);

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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// QUERYDB MAIN ////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool QueryDbMainLoop(Config* config,
                     QueryDatabase* db,
                     bool* exit_when_idle,
                     MultiQueueWaiter* waiter,
                     QueueManager* queue,
                     Project* project,
                     FileConsumer::SharedState* file_consumer_shared,
                     ImportManager* import_manager,
                     TimestampManager* timestamp_manager,
                     SemanticHighlightSymbolCache* semantic_cache,
                     WorkingFiles* working_files,
                     ClangCompleteManager* clang_complete,
                     IncludeComplete* include_complete,
                     CodeCompleteCache* global_code_complete_cache,
                     CodeCompleteCache* non_global_code_complete_cache,
                     CodeCompleteCache* signature_cache) {
  IpcManager* ipc = IpcManager::instance();

  bool did_work = false;

  std::vector<std::unique_ptr<BaseIpcMessage>> messages =
      ipc->for_querydb.DequeueAll();
  for (auto& message : messages) {
    did_work = true;

    for (MessageHandler* handler : *MessageHandler::message_handlers) {
      if (handler->GetId() == message->method_id) {
        handler->Run(std::move(message));
        break;
      }
    }
    if (message) {
      LOG_S(FATAL) << "Exiting; unhandled IPC message "
                   << IpcIdToString(message->method_id);
      exit(1);
    }
  }

  // TODO: consider rate-limiting and checking for IPC messages so we don't
  // block requests / we can serve partial requests.

  if (QueryDb_ImportMain(config, db, import_manager, queue, semantic_cache,
                         working_files)) {
    did_work = true;
  }

  return did_work;
}

void RunQueryDbThread(const std::string& bin_name,
                      Config* config,
                      MultiQueueWaiter* waiter,
                      QueueManager* queue) {
  bool exit_when_idle = false;
  Project project;
  SemanticHighlightSymbolCache semantic_cache;
  WorkingFiles working_files;
  FileConsumer::SharedState file_consumer_shared;

  ClangCompleteManager clang_complete(
      config, &project, &working_files,
      std::bind(&EmitDiagnostics, &working_files, std::placeholders::_1,
                std::placeholders::_2),
      std::bind(&IndexWithTuFromCodeCompletion, queue, &file_consumer_shared,
                std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4));

  IncludeComplete include_complete(config, &project);
  auto global_code_complete_cache = MakeUnique<CodeCompleteCache>();
  auto non_global_code_complete_cache = MakeUnique<CodeCompleteCache>();
  auto signature_cache = MakeUnique<CodeCompleteCache>();
  ImportManager import_manager;
  TimestampManager timestamp_manager;
  QueryDatabase db;

  // Setup shared references.
  for (MessageHandler* handler : *MessageHandler::message_handlers) {
    handler->config = config;
    handler->db = &db;
    handler->exit_when_idle = &exit_when_idle;
    handler->waiter = waiter;
    handler->queue = queue;
    handler->project = &project;
    handler->file_consumer_shared = &file_consumer_shared;
    handler->import_manager = &import_manager;
    handler->timestamp_manager = &timestamp_manager;
    handler->semantic_cache = &semantic_cache;
    handler->working_files = &working_files;
    handler->clang_complete = &clang_complete;
    handler->include_complete = &include_complete;
    handler->global_code_complete_cache = global_code_complete_cache.get();
    handler->non_global_code_complete_cache =
        non_global_code_complete_cache.get();
    handler->signature_cache = signature_cache.get();
  }

  // Run query db main loop.
  SetCurrentThreadName("querydb");
  while (true) {
    bool did_work = QueryDbMainLoop(
        config, &db, &exit_when_idle, waiter, queue, &project,
        &file_consumer_shared, &import_manager, &timestamp_manager,
        &semantic_cache, &working_files, &clang_complete, &include_complete,
        global_code_complete_cache.get(), non_global_code_complete_cache.get(),
        signature_cache.get());

    // No more work left and exit request. Exit.
    if (!did_work && exit_when_idle && WorkThread::num_active_threads == 0) {
      LOG_S(INFO) << "Exiting; exit_when_idle is set and there is no more work";
      exit(0);
    }

    // Cleanup and free any unused memory.
    FreeUnusedMemory();

    if (!did_work) {
      waiter->Wait({&IpcManager::instance()->for_querydb, &queue->do_id_map,
                    &queue->on_indexed});
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// STDIN MAIN //////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Separate thread whose only job is to read from stdin and
// dispatch read commands to the actual indexer program. This
// cannot be done on the main thread because reading from std::cin
// blocks.
//
// |ipc| is connected to a server.
void LaunchStdinLoop(Config* config,
                     std::unordered_map<IpcId, Timer>* request_times) {
  WorkThread::StartThread("stdin", [request_times]() {
    IpcManager* ipc = IpcManager::instance();

    std::unique_ptr<BaseIpcMessage> message =
        MessageRegistry::instance()->ReadMessageFromStdin(
            g_log_stdin_stdout_to_stderr);

    // Message parsing can fail if we don't recognize the method.
    if (!message)
      return WorkThread::Result::MoreWork;

    (*request_times)[message->method_id] = Timer();

    switch (message->method_id) {
      case IpcId::Initialized: {
        // TODO: don't send output until we get this notification
        break;
      }

      case IpcId::CancelRequest: {
        // TODO: support cancellation
        break;
      }

      case IpcId::Exit: {
        LOG_S(INFO) << "Exiting";
        exit(0);
        break;
      }

      case IpcId::CqueryExitWhenIdle: {
        // querydb needs to know to exit when idle. We return out of the stdin
        // loop to exit the thread. If we keep parsing input stdin is likely
        // closed so cquery will exit.
        LOG_S(INFO) << "cquery will exit when all threads are idle";
        ipc->for_querydb.Enqueue(std::move(message));
        return WorkThread::Result::ExitThread;
      }

      case IpcId::Initialize:
      case IpcId::TextDocumentDidOpen:
      case IpcId::CqueryTextDocumentDidView:
      case IpcId::TextDocumentDidChange:
      case IpcId::TextDocumentDidClose:
      case IpcId::TextDocumentDidSave:
      case IpcId::TextDocumentRename:
      case IpcId::TextDocumentCompletion:
      case IpcId::TextDocumentSignatureHelp:
      case IpcId::TextDocumentDefinition:
      case IpcId::TextDocumentDocumentHighlight:
      case IpcId::TextDocumentHover:
      case IpcId::TextDocumentReferences:
      case IpcId::TextDocumentDocumentSymbol:
      case IpcId::TextDocumentDocumentLink:
      case IpcId::TextDocumentCodeAction:
      case IpcId::TextDocumentCodeLens:
      case IpcId::WorkspaceSymbol:
      case IpcId::CqueryFreshenIndex:
      case IpcId::CqueryTypeHierarchyTree:
      case IpcId::CqueryCallTreeInitial:
      case IpcId::CqueryCallTreeExpand:
      case IpcId::CqueryVars:
      case IpcId::CqueryCallers:
      case IpcId::CqueryBase:
      case IpcId::CqueryDerived:
      case IpcId::CqueryIndexFile:
      case IpcId::CqueryQueryDbWaitForIdleIndexer: {
        ipc->for_querydb.Enqueue(std::move(message));
        break;
      }

      default: {
        LOG_S(ERROR) << "Unhandled IPC message "
                     << IpcIdToString(message->method_id);
        exit(1);
      }
    }

    return WorkThread::Result::MoreWork;
  });
}

void LaunchStdoutThread(std::unordered_map<IpcId, Timer>* request_times,
                        MultiQueueWaiter* waiter,
                        QueueManager* queue) {
  WorkThread::StartThread("stdout", [=]() {
    IpcManager* ipc = IpcManager::instance();

    std::vector<IpcManager::StdoutMessage> messages =
        ipc->for_stdout.DequeueAll();
    if (messages.empty()) {
      waiter->Wait({&ipc->for_stdout});
      return queue->HasWork() ? WorkThread::Result::MoreWork
                              : WorkThread::Result::NoWork;
    }

    for (auto& message : messages) {
      if (ShouldDisplayIpcTiming(message.id)) {
        Timer time = (*request_times)[message.id];
        time.ResetAndPrint("[e2e] Running " +
                           std::string(IpcIdToString(message.id)));
      }

      if (g_log_stdin_stdout_to_stderr) {
        std::ostringstream sstream;
        sstream << "[COUT] |";
        sstream << message.content;
        sstream << "|\n";
        std::cerr << sstream.str();
        std::cerr.flush();
      }

      std::cout << message.content;
      std::cout.flush();
    }

    return WorkThread::Result::MoreWork;
  });
}

void LanguageServerMain(const std::string& bin_name,
                        Config* config,
                        MultiQueueWaiter* waiter) {
  QueueManager queue(waiter);
  std::unordered_map<IpcId, Timer> request_times;

  std::cin.tie(NULL);
  LaunchStdinLoop(config, &request_times);

  // We run a dedicated thread for writing to stdout because there can be an
  // unknown number of delays when output information.
  LaunchStdoutThread(&request_times, waiter, &queue);

  // Start querydb which takes over this thread. The querydb will launch
  // indexer threads as needed.
  RunQueryDbThread(bin_name, config, waiter, &queue);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// MAIN ////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
  loguru::init(argc, argv);
  loguru::add_file("cquery_diagnostics.log", loguru::Truncate,
                   loguru::Verbosity_MAX);
  loguru::g_flush_interval_ms = 0;
  loguru::g_stderr_verbosity = 1;

  MultiQueueWaiter waiter;
  IpcManager::CreateInstance(&waiter);

  // bool loop = true;
  // while (loop)
  //  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  // std::this_thread::sleep_for(std::chrono::seconds(10));

  PlatformInit();
  IndexInit();

  std::unordered_map<std::string, std::string> options =
      ParseOptions(argc, argv);

  bool print_help = true;

  if (HasOption(options, "--clang-sanity-check")) {
    print_help = false;
    ClangSanityCheck();
  }

  if (HasOption(options, "--log-stdin-stdout-to-stderr"))
    g_log_stdin_stdout_to_stderr = true;

  if (HasOption(options, "--test-unit")) {
    print_help = false;
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    int res = context.run();
    if (context.shouldExit())
      return res;
  }

  if (HasOption(options, "--test-index")) {
    print_help = false;
    RunIndexTests();
#if defined(_WIN32)
    std::cerr << std::endl << "[Enter] to exit" << std::endl;
    std::cin.get();
#endif
  }

  if (HasOption(options, "--language-server")) {
    print_help = false;
    // std::cerr << "Running language server" << std::endl;
    auto config = MakeUnique<Config>();
    LanguageServerMain(argv[0], config.get(), &waiter);
    return 0;
  }

  if (print_help) {
    std::cout << R"help(cquery help:

  cquery is a low-latency C++ language server.

  General:
    --help        Print this help information.
    --language-server
                  Run as a language server. This implements the language
                  server spec over STDIN and STDOUT.
    --test-unit   Run unit tests.
    --test-index  Run index tests.
    --log-stdin-stdout-to-stderr
                  Print stdin and stdout messages to stderr. This is a aid for
                  developing new language clients, as it makes it easier to
                  figure out how the client is interacting with cquery.
    --clang-sanity-check
                  Run a simple index test. Verifies basic clang functionality.
                  Needs to be executed from the cquery root checkout directory.

  Configuration:
    When opening up a directory, cquery will look for a compile_commands.json
    file emitted by your preferred build system. If not present, cquery will
    use a recursive directory listing instead. Command line flags can be
    provided by adding a file named `.cquery` in the top-level directory. Each
    line in that file is a separate argument.

    There are also a number of configuration options available when
    initializing the language server - your editor should have tooling to
    describe those options. See |Config| in this source code for a detailed
    list of all currently supported options.
)help";
  }

  return 0;
}
