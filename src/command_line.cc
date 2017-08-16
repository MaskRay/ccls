// TODO: cleanup includes
#include "buffer.h"
#include "cache.h"
#include "clang_complete.h"
#include "file_consumer.h"
#include "match.h"
#include "include_complete.h"
#include "ipc_manager.h"
#include "indexer.h"
#include "message_queue.h"
#include "query.h"
#include "query_utils.h"
#include "language_server_api.h"
#include "lex_utils.h"
#include "options.h"
#include "project.h"
#include "platform.h"
#include "serializer.h"
#include "standard_includes.h"
#include "test.h"
#include "timer.h"
#include "threaded_queue.h"
#include "working_files.h"

#include <loguru.hpp>
#include "tiny-process-library/process.hpp"

#include <doctest/doctest.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

#include <climits>
#include <fstream>
#include <future>
#include <functional>
#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <vector>

// TODO: provide a feature like 'https://github.com/goldsborough/clang-expand',
// ie, a fully linear view of a function with inline function calls expanded.
// We can probably use vscode decorators to achieve it.

// TODO: implement ThreadPool type which monitors CPU usage / number of work items
// per second completed and scales up/down number of running threads.

//namespace {

std::vector<std::string> kEmptyArgs;

// Expected client version. We show an error if this doesn't match.
const int kExpectedClientVersion = 3;





























// Cached completion information, so we can give fast completion results when
// the user erases a character. vscode will resend the completion request if
// that happens.
struct CodeCompleteCache {
  // NOTE: Make sure to access these variables under |WithLock|.
  optional<std::string> cached_path_;
  optional<lsPosition> cached_completion_position_;
  NonElidedVector<lsCompletionItem> cached_results_;

  std::mutex mutex_;

  void WithLock(std::function<void()> action) {
    std::lock_guard<std::mutex> lock(mutex_);
    action();
  }

  bool IsCacheValid(lsTextDocumentPositionParams position) {
    std::lock_guard<std::mutex> lock(mutex_);
    return cached_path_ == position.textDocument.uri.GetPath() &&
           cached_completion_position_ == position.position;
  }
};






















// This function returns true if e2e timing should be displayed for the given IpcId.
bool ShouldDisplayIpcTiming(IpcId id) {
  switch (id) {
  case IpcId::TextDocumentPublishDiagnostics:
  case IpcId::CqueryPublishInactiveRegions:
    return false;
  default:
    return true;
  }
}
































void PushBack(NonElidedVector<lsLocation>* result, optional<lsLocation> location) {
  if (location)
    result->push_back(*location);
}

bool FindFileOrFail(QueryDatabase* db, lsRequestId id, const std::string& absolute_path, QueryFile** out_query_file, QueryFileId* out_file_id = nullptr) {
  auto it = db->usr_to_file.find(LowerPathIfCaseInsensitive(absolute_path));
  if (it != db->usr_to_file.end()) {
    optional<QueryFile>& file = db->files[it->second.id];
    if (file) {
      *out_query_file = &file.value();
      if (out_file_id)
        *out_file_id = QueryFileId(it->second.id);
      return true;
    }
  }

  if (out_file_id)
    *out_file_id = QueryFileId((size_t)-1);

  LOG_S(INFO) << "Unable to find file " << absolute_path;

  Out_Error out;
  out.id = id;
  out.error.code = lsErrorCodes::InternalError;
  out.error.message = "Unable to find file " + absolute_path;
  IpcManager::instance()->SendOutMessageToClient(IpcId::Cout, out);

  return false;
}

QueryFile* FindFile(QueryDatabase* db, const std::string& absolute_path) {
  auto it = db->usr_to_file.find(LowerPathIfCaseInsensitive(absolute_path));
  if (it != db->usr_to_file.end()) {
    optional<QueryFile>& file = db->files[it->second.id];
    if (file)
      return &file.value();
  }
  return nullptr;
}


void PublishInactiveLines(WorkingFile* working_file, const std::vector<Range>& inactive) {
  Out_CquerySetInactiveRegion out;
  out.params.uri = lsDocumentUri::FromPath(working_file->filename);
  for (Range skipped : inactive) {
    optional<lsRange> ls_skipped = GetLsRange(working_file, skipped);
    if (ls_skipped)
      out.params.inactiveRegions.push_back(*ls_skipped);
  }
  IpcManager::instance()->SendOutMessageToClient(IpcId::CqueryPublishInactiveRegions, out);
}



optional<int> FindIncludeLine(const std::vector<std::string>& lines, const std::string& full_include_line) {
  //
  // This returns an include line. For example,
  //
  //    #include <a>  // 0
  //    #include <c>  // 1
  //
  // Given #include <b>, this will return '1', which means that the
  // #include <b> text should be inserted at the start of line 1. Inserting
  // at the start of a line allows insertion at both the top and bottom of the
  // document.
  //
  // If the include line is already in the document this returns nullopt.
  //

  optional<int> last_include_line;
  optional<int> best_include_line;

  //  1 => include line is gt content (ie, it should go after)
  // -1 => include line is lt content (ie, it should go before)
  int last_line_compare = 1;

  for (int line = 0; line < (int)lines.size(); ++line) {
    if (!StartsWith(lines[line], "#include")) {
      last_line_compare = 1;
      continue;
    }

    last_include_line = line;

    int current_line_compare = full_include_line.compare(lines[line]);
    if (current_line_compare == 0)
      return nullopt;

    if (last_line_compare == 1 && current_line_compare == -1)
      best_include_line = line;
    last_line_compare = current_line_compare;
  }

  if (best_include_line)
    return *best_include_line;
  // If |best_include_line| didn't match that means we likely didn't find an
  // include which was lt the new one, so put it at the end of the last include
  // list.
  if (last_include_line)
    return *last_include_line + 1;
  // No includes, use top of document.
  return 0;
}

optional<QueryFileId> GetImplementationFile(QueryDatabase* db, QueryFileId file_id, QueryFile* file) {
  for (SymbolRef sym : file->def.outline) {
    switch (sym.idx.kind) {
      case SymbolKind::Func: {
        optional<QueryFunc>& func = db->funcs[sym.idx.idx];
        // Note: we ignore the definition if it is in the same file (ie, possibly a header).
        if (func && func->def.definition_extent && func->def.definition_extent->path != file_id)
          return func->def.definition_extent->path;
        break;
      }
      case SymbolKind::Var: {
        optional<QueryVar>& var = db->vars[sym.idx.idx];
        // Note: we ignore the definition if it is in the same file (ie, possibly a header).
        if (var && var->def.definition_extent && var->def.definition_extent->path != file_id)
          return db->vars[sym.idx.idx]->def.definition_extent->path;
        break;
      }
      default:
        break;
    }
  }

  // No associated definition, scan the project for a file in the same
  // directory with the same base-name.
  std::string original_path = LowerPathIfCaseInsensitive(file->def.path);
  std::string target_path = original_path;
  size_t last = target_path.find_last_of('.');
  if (last != std::string::npos) {
    target_path = target_path.substr(0, last);
  }

  LOG_S(INFO) << "!! Looking for impl file that starts with " << target_path;

  for (auto& entry : db->usr_to_file) {
    Usr path = entry.first;

    // Do not consider header files for implementation files.
    // TODO: make file extensions configurable.
    if (EndsWith(path, ".h") || EndsWith(path, ".hpp"))
      continue;

    if (StartsWith(path, target_path) && path != original_path) {
      return entry.second;
    }
  }

  return nullopt;
}

void EnsureImplFile(QueryDatabase* db, QueryFileId file_id, optional<lsDocumentUri>& impl_uri, optional<QueryFileId>& impl_file_id) {
  if (!impl_uri.has_value()) {
    optional<QueryFile>& file = db->files[file_id.id];
    assert(file);

    impl_file_id = GetImplementationFile(db, file_id, &file.value());
    if (!impl_file_id.has_value())
      impl_file_id = file_id;

    optional<QueryFile>& impl_file = db->files[impl_file_id->id];
    if (impl_file)
      impl_uri = lsDocumentUri::FromPath(impl_file->def.path);
    else
      impl_uri = lsDocumentUri::FromPath(file->def.path);
  }
}

optional<lsTextEdit> BuildAutoImplementForFunction(QueryDatabase* db, WorkingFiles* working_files, WorkingFile* working_file, int default_line, QueryFileId decl_file_id, QueryFileId impl_file_id, QueryFunc& func) {
  for (const QueryLocation& decl : func.declarations) {
    if (decl.path != decl_file_id)
      continue;

    optional<lsRange> ls_decl = GetLsRange(working_file, decl.range);
    if (!ls_decl)
      continue;

    optional<std::string> type_name;
    optional<lsPosition> same_file_insert_end;
    if (func.def.declaring_type) {
      optional<QueryType>& declaring_type = db->types[func.def.declaring_type->id];
      if (declaring_type) {
        type_name = declaring_type->def.short_name;
        optional<lsRange> ls_type_def_extent = GetLsRange(working_file, declaring_type->def.definition_extent->range);
        if (ls_type_def_extent) {
          same_file_insert_end = ls_type_def_extent->end;
          same_file_insert_end->character += 1; // move past semicolon.
        }
      }
    }

    std::string insert_text;
    int newlines_after_name = 0;
    LexFunctionDeclaration(working_file->buffer_content, ls_decl->start, type_name, &insert_text, &newlines_after_name);

    if (!same_file_insert_end) {
      same_file_insert_end = ls_decl->end;
      same_file_insert_end->line += newlines_after_name;
      same_file_insert_end->character = 1000;
    }

    lsTextEdit edit;

    if (decl_file_id == impl_file_id) {
      edit.range.start = *same_file_insert_end;
      edit.range.end = *same_file_insert_end;
      edit.newText = "\n\n" + insert_text;
    }
    else {
      lsPosition best_pos;
      best_pos.line = default_line;
      int best_dist = INT_MAX;

      optional<QueryFile>& file = db->files[impl_file_id.id];
      assert(file);
      for (SymbolRef sym : file->def.outline) {
        switch (sym.idx.kind) {
          case SymbolKind::Func: {
            optional<QueryFunc>& sym_func = db->funcs[sym.idx.idx];
            if (!sym_func || !sym_func->def.definition_extent)
              break;

            for (QueryLocation& func_decl : sym_func->declarations) {
              if (func_decl.path == decl_file_id) {
                int dist = func_decl.range.start.line - decl.range.start.line;
                if (abs(dist) < abs(best_dist)) {
                  optional<lsLocation> def_loc = GetLsLocation(db, working_files, *sym_func->def.definition_extent);
                  if (!def_loc)
                    continue;

                  best_dist = dist;

                  if (dist > 0)
                    best_pos = def_loc->range.start;
                  else
                    best_pos = def_loc->range.end;
                }
              }
            }

            break;
          }
          case SymbolKind::Var: {
            // TODO: handle vars.

            //optional<QueryVar>& var = db->vars[sym.idx.idx];
            //if (!var || !var->def.definition_extent)
            //  continue;

            break;
          }
          case SymbolKind::Invalid:
          case SymbolKind::File:
          case SymbolKind::Type:
            LOG_S(WARNING) << "Unexpected SymbolKind "
                           << static_cast<int>(sym.idx.kind);
            break;
        }
      }


      edit.range.start = best_pos;
      edit.range.end = best_pos;
      if (best_dist < 0)
        edit.newText = "\n\n" + insert_text;
      else
        edit.newText = insert_text + "\n\n";
    }

    return edit;
  }

  return nullopt;
}

void EmitDiagnostics(WorkingFiles* working_files, std::string path, NonElidedVector<lsDiagnostic> diagnostics) {
  // Emit diagnostics.
  Out_TextDocumentPublishDiagnostics diagnostic_response;
  diagnostic_response.params.uri = lsDocumentUri::FromPath(path);
  diagnostic_response.params.diagnostics = diagnostics;
  IpcManager::instance()->SendOutMessageToClient(IpcId::TextDocumentPublishDiagnostics, diagnostic_response);

  // Cache diagnostics so we can show fixits.
  working_files->DoActionOnFile(path, [&](WorkingFile* working_file) {
    if (working_file)
      working_file->diagnostics_ = diagnostics;
  });
}

// Pre-filters completion responses before sending to vscode. This results in a
// significantly snappier completion experience as vscode is easily overloaded
// when given 1000+ completion items.
void FilterCompletionResponse(Out_TextDocumentComplete* complete_response,
                              const std::string& complete_text) {
  // Used to inject more completions.
#if false
  const size_t kNumIterations = 250;
  size_t size = complete_response->result.items.size();
  complete_response->result.items.reserve(size * (kNumIterations + 1));
  for (size_t iteration = 0; iteration < kNumIterations; ++iteration) {
    for (size_t i = 0; i < size; ++i) {
      auto item = complete_response->result.items[i];
      item.label += "#" + std::to_string(iteration);
      complete_response->result.items.push_back(item);
    }
  }
#endif


  const size_t kMaxResultSize = 100u;
  if (complete_response->result.items.size() > kMaxResultSize) {
    //std::cerr << "!!! Filtering " << complete_response->result.items.size() << " results using " << complete_text << std::endl;

    complete_response->result.isIncomplete = true;

    if (complete_text.empty()) {
      complete_response->result.items.resize(kMaxResultSize);
    }
    else {
      NonElidedVector<lsCompletionItem> filtered_result;
      filtered_result.reserve(kMaxResultSize);

      std::unordered_set<std::string> inserted;
      inserted.reserve(kMaxResultSize);

      for (const lsCompletionItem& item : complete_response->result.items) {
        if (item.label.find(complete_text) != std::string::npos) {
          // Don't insert the same completion entry.
          if (!inserted.insert(item.InsertedContent()).second)
            continue;

          filtered_result.push_back(item);
          if (filtered_result.size() >= kMaxResultSize)
            break;
        }
      }

      if (filtered_result.size() < kMaxResultSize) {
        for (const lsCompletionItem& item : complete_response->result.items) {
          if (SubstringMatch(complete_text, item.label)) {
            // Don't insert the same completion entry.
            if (!inserted.insert(item.InsertedContent()).second)
              continue;

            filtered_result.push_back(item);
            if (filtered_result.size() >= kMaxResultSize)
              break;
          }
        }
      }

      complete_response->result.items = filtered_result;
    }

    //std::cerr << "!! Filtering resulted in " << complete_response->result.items.size() << " entries" << std::endl;
  }
}


















struct Index_Request {
  std::string path;
  std::vector<std::string> args; // TODO: make this a string that is parsed lazily.

  Index_Request(const std::string& path, const std::vector<std::string>& args)
    : path(path), args(args) {}
};

struct Index_DoIdMap {
  std::unique_ptr<IndexFile> current;
  std::unique_ptr<IndexFile> previous;

  PerformanceImportFile perf;
  bool is_interactive = false;
  bool load_previous = false;

  Index_DoIdMap(
      std::unique_ptr<IndexFile> current,
      PerformanceImportFile perf,
      bool is_interactive)
    : current(std::move(current)),
      perf(perf),
      is_interactive(is_interactive) {}
};

struct Index_OnIdMapped {
  struct File {
    std::unique_ptr<IndexFile> file;
    std::unique_ptr<IdMap> ids;

    File(std::unique_ptr<IndexFile> file, std::unique_ptr<IdMap> ids)
      : file(std::move(file)), ids(std::move(ids)) {}
  };

  std::unique_ptr<File> previous;
  std::unique_ptr<File> current;

  PerformanceImportFile perf;
  bool is_interactive;

  Index_OnIdMapped(
      PerformanceImportFile perf,
      bool is_interactive)
    : perf(perf),
      is_interactive(is_interactive) {}
};

struct Index_OnIndexed {
  IndexUpdate update;
  PerformanceImportFile perf;

  Index_OnIndexed(
      IndexUpdate& update,
      PerformanceImportFile perf)
    : update(update), perf(perf) {}
};







































struct QueueManager {
  using Index_RequestQueue = ThreadedQueue<Index_Request>;
  using Index_DoIdMapQueue = ThreadedQueue<Index_DoIdMap>;
  using Index_OnIdMappedQueue = ThreadedQueue<Index_OnIdMapped>;
  using Index_OnIndexedQueue = ThreadedQueue<Index_OnIndexed>;

  Index_RequestQueue index_request;
  Index_DoIdMapQueue do_id_map;
  Index_DoIdMapQueue load_previous_index;
  Index_OnIdMappedQueue on_id_mapped;
  Index_OnIndexedQueue on_indexed;

  QueueManager(MultiQueueWaiter* waiter) : index_request(waiter), do_id_map(waiter), load_previous_index(waiter), on_id_mapped(waiter), on_indexed(waiter) {}
};

void RegisterMessageTypes() {
  MessageRegistry::instance()->Register<Ipc_CancelRequest>();
  MessageRegistry::instance()->Register<Ipc_InitializeRequest>();
  MessageRegistry::instance()->Register<Ipc_InitializedNotification>();
  MessageRegistry::instance()->Register<Ipc_Exit>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDidOpen>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDidChange>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDidClose>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDidSave>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentRename>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentComplete>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentSignatureHelp>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDefinition>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDocumentHighlight>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentHover>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentReferences>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDocumentSymbol>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDocumentLink>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentCodeAction>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentCodeLens>();
  MessageRegistry::instance()->Register<Ipc_CodeLensResolve>();
  MessageRegistry::instance()->Register<Ipc_WorkspaceSymbol>();
  MessageRegistry::instance()->Register<Ipc_CqueryFreshenIndex>();
  MessageRegistry::instance()->Register<Ipc_CqueryTypeHierarchyTree>();
  MessageRegistry::instance()->Register<Ipc_CqueryCallTreeInitial>();
  MessageRegistry::instance()->Register<Ipc_CqueryCallTreeExpand>();
  MessageRegistry::instance()->Register<Ipc_CqueryVars>();
  MessageRegistry::instance()->Register<Ipc_CqueryCallers>();
  MessageRegistry::instance()->Register<Ipc_CqueryBase>();
  MessageRegistry::instance()->Register<Ipc_CqueryDerived>();
}







#if false
  TODO: re-enable
  void PriorityEnqueueFileForIndex(QueryDatabase* db, Project* project, Index_DoIndexQueue* queue_do_index, WorkingFile* working_file, const std::string& path) {
    // Only do a delta update (Type::Parse) if we've already imported the
    // file. If the user saves a file not loaded by the project we don't
    // want the initial import to be a delta-update.
    Index_DoIndex::Type index_type = Index_DoIndex::Type::Parse;
    // TODO/FIXME: this is racy. we need to check if the file is already in the import pipeline. So we should change PriorityEnqueue to look at existing contents before appending. That's not a full fix tho.
    QueryFile* file = FindFile(db, path);
    if (!file)
      index_type = Index_DoIndex::Type::ImportThenParse;

    queue_do_index->PriorityEnqueue(Index_DoIndex(index_type, project->FindCompilationEntryForFile(path), working_file->buffer_content, true /*is_interactive*/));
  }
#endif

void InsertSymbolIntoResult(QueryDatabase* db, WorkingFiles* working_files, SymbolIdx symbol, std::vector<lsSymbolInformation>* result) {
  optional<lsSymbolInformation> info = GetSymbolInfo(db, working_files, symbol);
  if (!info)
    return;

  optional<QueryLocation> location = GetDefinitionExtentOfSymbol(db, symbol);
  if (!location) {
    auto decls = GetDeclarationsOfSymbolForGotoDefinition(db, symbol);
    if (decls.empty())
      return;
    location = decls[0];
  }

  optional<lsLocation> ls_location = GetLsLocation(db, working_files, *location);
  if (!ls_location)
    return;
  info->location = *ls_location;
  result->push_back(*info);
}

// Manages loading caches from file paths for the indexer process.
struct CacheLoader {
  explicit CacheLoader(Config* config) : config_(config) {}

  IndexFile* TryLoad(const std::string& path) {
    auto it = caches.find(path);
    if (it != caches.end())
      return it->second.get();

    std::unique_ptr<IndexFile> cache = LoadCachedIndex(config_, path);
    if (!cache)
      return nullptr;

    caches[path] = std::move(cache);
    return caches[path].get();
  }

  std::unique_ptr<IndexFile> TryTakeOrLoad(const std::string& path) {
    auto it = caches.find(path);
    if (it != caches.end()) {
      auto result = std::move(it->second);
      caches.erase(it);
      return result;
    }

    return LoadCachedIndex(config_, path);
  }

  std::unordered_map<std::string, std::unique_ptr<IndexFile>> caches;
  Config* config_;
};

struct IndexManager {
  std::unordered_set<std::string> files_being_indexed_;
  std::mutex mutex_;

  // Marks a file as being indexed. Returns true if the file is not already
  // being indexed.
  bool MarkIndex(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    return files_being_indexed_.insert(path).second;
  }

  // Unmarks a file as being indexed, so it can get indexed again in the
  // future.
  void ClearIndex(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = files_being_indexed_.find(path);
    assert(it != files_being_indexed_.end());
    files_being_indexed_.erase(it);
  }
};


//}  // namespace

std::vector<Index_DoIdMap> DoParseFile(
  Config* config,
  clang::Index* index,
  FileConsumer::SharedState* file_consumer_shared,
  CacheLoader* cache_loader,
  const std::string& path,
  const std::vector<std::string>& args) {
  std::vector<Index_DoIdMap> result;

  IndexFile* previous_index = cache_loader->TryLoad(path);
  if (previous_index) {
    // If none of the dependencies have changed, skip parsing and just load from cache.
    auto file_needs_parse = [&](const std::string& path) {
      int64_t modification_timestamp = GetLastModificationTime(path);
      // PERF: We don't need to fully deserialize the file from cache here; we
      // need to load it into memory but after that we can just check the
      // timestamp. We may want to actually introduce a third file, so we have
      // foo.cc, foo.cc.index.json, and foo.cc.meta.json and store the
      // timestamp in foo.cc.meta.json. We may also want to begin writing
      // to separate folders so things are not all in one folder (ie, take the
      // first 5 chars as the folder name).
      IndexFile* index = cache_loader->TryLoad(path);
      if (!index || modification_timestamp != index->last_modification_time) {
        file_consumer_shared->Reset(path);
        return true;
      }
      return false;
    };

    // Check timestamps and update |file_consumer_shared|.
    bool needs_reparse = file_needs_parse(path);
    for (const std::string& dependency : previous_index->dependencies) {
      assert(!dependency.empty());

      if (file_needs_parse(dependency)) {
        LOG_S(INFO) << "Timestamp has changed for " << dependency;
        needs_reparse = true;
        // SUBTLE: Do not break here, as |file_consumer_shared| is updated
        // inside of |file_needs_parse|.
      }
    }

    // No timestamps changed - load directly from cache.
    if (!needs_reparse) {
      LOG_S(INFO) << "Skipping parse; no timestamp change for " << path;

      // TODO/FIXME: real is_interactive
      bool is_interactive = false;
      // TODO/FIXME: real perf
      PerformanceImportFile perf;
      result.push_back(Index_DoIdMap(cache_loader->TryTakeOrLoad(path), perf, is_interactive));
      for (const std::string& dependency : previous_index->dependencies) {
        // Only actually load the file if we haven't loaded it yet. Important
        // for perf when files have lots of common dependencies.
        if (!file_consumer_shared->Mark(dependency))
          continue;

        LOG_S(INFO) << "Emitting index result for " << dependency;
        result.push_back(Index_DoIdMap(cache_loader->TryTakeOrLoad(dependency), perf, is_interactive));
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
  std::vector<FileContents> file_contents;
  for (const auto& it : cache_loader->caches) {
    const std::unique_ptr<IndexFile>& index = it.second;
    assert(index);
    optional<std::string> index_content = ReadContent(index->path);
    if (!index_content) {
      LOG_S(ERROR) << "Failed to preload index content for " << index->path;
      continue;
    }
    file_contents.push_back(FileContents(index->path, *index_content));
  }

  PerformanceImportFile perf;
  std::vector<std::unique_ptr<IndexFile>> indexes = Parse(
    config, file_consumer_shared,
    path, args, file_contents,
    &perf, index);

  for (std::unique_ptr<IndexFile>& new_index : indexes) {
    Timer time;

    // TODO: don't load cached index. We don't need to do this when indexer always exports dependency tree.
    // Sanity check that verifies we did not generate a new index for a file whose timestamp did not change.
    //{
    //  IndexFile* previous_index = cache_loader->TryLoad(new_index->path);
    //  assert(!previous_index || GetLastModificationTime(new_index->path) != previous_index->last_modification_time);
    //}

    // Note: we are reusing the parent perf.
    perf.index_load_cached = time.ElapsedMicrosecondsAndReset();

    // TODO/FIXME: real is_interactive
    bool is_interactive = false;
    LOG_S(INFO) << "Emitting index result for " << new_index->path;
    result.push_back(Index_DoIdMap(std::move(new_index), perf, is_interactive));
  }

  return result;
}


// TODO: import to CACHE_DIR/staging/foo.cc
// TODO: split index files into foo.cc.json, foo.cc.timestamp, foo.cc


std::vector<Index_DoIdMap> ParseFile(
    Config* config,
    clang::Index* index,
    FileConsumer::SharedState* file_consumer_shared,
    const Project::Entry& entry) {

  CacheLoader cache_loader(config);

  // Try to determine the original import file by loading the file from cache.
  // This lets the user request an index on a header file, which clang will
  // complain about if indexed by itself.
  IndexFile* entry_cache = cache_loader.TryLoad(entry.filename);
  std::string tu_path = entry_cache ? entry_cache->import_file : entry.filename;
  return DoParseFile(config, index, file_consumer_shared, &cache_loader, tu_path, entry.args);
}

bool IndexMain_DoParse(
    Config* config,
    QueueManager* queue,
    FileConsumer::SharedState* file_consumer_shared,
    clang::Index* index) {

  optional<Index_Request> request = queue->index_request.TryDequeue();
  if (!request)
    return false;

  Project::Entry entry;
  entry.filename = request->path;
  entry.args = request->args;
  std::vector<Index_DoIdMap> responses = ParseFile(config, index, file_consumer_shared, entry);

  // TODO/FIXME: bulk enqueue so we don't lock so many times
  for (Index_DoIdMap& response : responses)
    queue->do_id_map.Enqueue(std::move(response));

  return !responses.empty();
}

bool IndexMain_DoCreateIndexUpdate(
    Config* config,
    QueueManager* queue) {
  // TODO: Index_OnIdMapped dtor is failing because it seems that its contents have already been destroyed.
  optional<Index_OnIdMapped> response = queue->on_id_mapped.TryDequeue();
  if (!response)
    return false;

  Timer time;

  IdMap* previous_id_map = nullptr;
  IndexFile* previous_index = nullptr;
  if (response->previous) {
    LOG_S(INFO) << "Creating delta update for " << response->previous->file->path;
    previous_id_map = response->previous->ids.get();
    previous_index = response->previous->file.get();
  }

  // Build delta update.
  IndexUpdate update = IndexUpdate::CreateDelta(
      previous_id_map, response->current->ids.get(),
      previous_index, response->current->file.get());
  response->perf.index_make_delta = time.ElapsedMicrosecondsAndReset();

  // Write new cache to disk if it is a fresh index.
  if (!response->current->file->is_loaded_from_cache_) {
    time.Reset();
    WriteToCache(config, *response->current->file);
    response->perf.index_save_to_disk = time.ElapsedMicrosecondsAndReset();
  }

#if false
#define PRINT_SECTION(name) \
  if (response->perf.name) {\
    total += response->perf.name; \
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
  output << std::endl;
  std::cerr << output.rdbuf();
#undef PRINT_SECTION

  if (response->is_interactive)
    std::cerr << "Applying IndexUpdate" << std::endl << update.ToString() << std::endl;
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
  LOG_IF_S(ERROR, !response->previous) << "Unable to load previous index for already imported index " << response->current->path;

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
    time.ResetAndPrint("Joining two querydb updates");
  }
}

void IndexMain(Config* config,
               FileConsumer::SharedState* file_consumer_shared,
               Project* project,
               WorkingFiles* working_files,
               MultiQueueWaiter* waiter,
               QueueManager* queue) {
  SetCurrentThreadName("indexer");
  // TODO: dispose of index after it is not used for a while.
  clang::Index index(1, 0);

  while (true) {
    // TODO: process all off IndexMain_DoIndex before calling
    // IndexMain_DoCreateIndexUpdate for
    //       better icache behavior. We need to have some threads spinning on
    //       both though
    //       otherwise memory usage will get bad.

    // We need to make sure to run both IndexMain_DoParse and
    // IndexMain_DoCreateIndexUpdate so we don't starve querydb from doing any
    // work. Running both also lets the user query the partially constructed
    // index.
    bool did_parse = IndexMain_DoParse(config, queue, file_consumer_shared, &index);

    bool did_create_update =
        IndexMain_DoCreateIndexUpdate(config, queue);

    bool did_load_previous = IndexMain_LoadPreviousIndex(config, queue);

    // Nothing to index and no index updates to create, so join some already
    // created index updates to reduce work on querydb thread.
    bool did_merge = false;
    if (!did_parse && !did_create_update && !did_load_previous)
      did_merge = IndexMergeIndexUpdates(queue);

    // We didn't do any work, so wait for a notification.
    if (!did_parse && !did_create_update && !did_merge && !did_load_previous) {
      waiter->Wait(
          {&queue->index_request, &queue->on_id_mapped, &queue->load_previous_index, &queue->on_indexed});
    }
  }
}

bool QueryDb_ImportMain(Config* config, QueryDatabase* db, QueueManager* queue, WorkingFiles* working_files) {
  bool did_work = false;

  while (true) {
    optional<Index_DoIdMap> request = queue->do_id_map.TryDequeue();
    if (!request)
      break;
    did_work = true;

    // If the request does not have previous state and we have already imported
    // it, load the previous state from disk and rerun IdMap logic later. Do not
    // do this if we have already attempted in the past.
    if (!request->load_previous &&
      !request->previous &&
      db->usr_to_file.find(LowerPathIfCaseInsensitive(request->current->path)) != db->usr_to_file.end()) {
      assert(!request->load_previous);
      request->load_previous = true;
      queue->load_previous_index.Enqueue(std::move(*request));
      continue;
    }

    Index_OnIdMapped response(request->perf, request->is_interactive);
    Timer time;

    assert(request->current);

    auto make_map = [db](std::unique_ptr<IndexFile> file) -> std::unique_ptr<Index_OnIdMapped::File> {
      if (!file)
        return nullptr;

      auto id_map = MakeUnique<IdMap>(db, file->id_cache);
      return MakeUnique<Index_OnIdMapped::File>(std::move(file), std::move(id_map));
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
      WorkingFile* working_file = working_files->GetFileByFilename(updated_file.path);
      if (working_file) {
        optional<std::string> cached_file_contents = LoadCachedFileContents(config, updated_file.path);
        if (cached_file_contents)
          working_file->SetIndexContent(*cached_file_contents);
        else
          working_file->SetIndexContent(working_file->buffer_content);
        time.ResetAndPrint("Update WorkingFile index contents (via disk load) for " + updated_file.path);
      }
    }

    db->ApplyIndexUpdate(&response->update);
    //time.ResetAndPrint("[querydb] Applying index update");
  }

  return did_work;
}












































































bool QueryDbMainLoop(
    Config* config,
    QueryDatabase* db,
    MultiQueueWaiter* waiter,
    QueueManager* queue,
    Project* project,
    FileConsumer::SharedState* file_consumer_shared,
    WorkingFiles* working_files,
    ClangCompleteManager* clang_complete,
    IncludeComplete* include_complete,
    CodeCompleteCache* global_code_complete_cache,
    CodeCompleteCache* non_global_code_complete_cache,
    CodeCompleteCache* signature_cache) {
  IpcManager* ipc = IpcManager::instance();

  bool did_work = false;

  std::vector<std::unique_ptr<BaseIpcMessage>> messages = ipc->GetMessages(IpcManager::Destination::Server);
  for (auto& message : messages) {
    did_work = true;
    //std::cerr << "[querydb] Processing message " << IpcIdToString(message->method_id) << std::endl;

    switch (message->method_id) {
      case IpcId::Initialize: {
        auto request = static_cast<Ipc_InitializeRequest*>(message.get());

        // Log initialization parameters.
        rapidjson::StringBuffer output;
        Writer writer(output);
        Reflect(writer, request->params.initializationOptions);
        std::cerr << output.GetString() << std::endl;

        if (request->params.rootUri) {
          std::string project_path = request->params.rootUri->GetPath();
          LOG_S(INFO) << "[querydb] Initialize in directory " << project_path
            << " with uri " << request->params.rootUri->raw_uri;

          if (!request->params.initializationOptions) {
            LOG_S(INFO) << "Initialization parameters (particularily cacheDirectory) are required";
            exit(1);
          }

          *config = *request->params.initializationOptions;

          // Check client version.
          if (config->clientVersion != kExpectedClientVersion) {
            Out_ShowLogMessage out;
            out.display_type = Out_ShowLogMessage::DisplayType::Show;
            out.params.type = lsMessageType::Error;
            out.params.message = "cquery client (v" + std::to_string(config->clientVersion) + ") and server (v" + std::to_string(kExpectedClientVersion) + ") version mismatch. Please update ";
            if (config->clientVersion > kExpectedClientVersion)
              out.params.message += "the cquery binary.";
            else
              out.params.message += "your extension client (VSIX file). Make sure to uninstall the cquery extension and restart vscode before reinstalling.";
            out.Write(std::cout);
          }

          // Make sure cache directory is valid.
          if (config->cacheDirectory.empty()) {
            LOG_S(ERROR) << "No cache directory";
            exit(1);
          }
          config->cacheDirectory = NormalizePath(config->cacheDirectory);
          EnsureEndsInSlash(config->cacheDirectory);
          MakeDirectoryRecursive(config->cacheDirectory);

          // Set project root.
          config->projectRoot = NormalizePath(request->params.rootUri->GetPath());
          EnsureEndsInSlash(config->projectRoot);

          // Start indexer threads.
          // Set default indexer count if not specified.
          if (config->indexerCount == 0) {
            config->indexerCount = std::max<int>(std::thread::hardware_concurrency(), 2) - 1;
          }
          std::cerr << "[querydb] Starting " << config->indexerCount << " indexers" << std::endl;
          for (int i = 0; i < config->indexerCount; ++i) {
            new std::thread([&]() {
              IndexMain(config, file_consumer_shared, project, working_files, waiter, queue);
            });
          }

          Timer time;

          // Open up / load the project.
          project->Load(config->extraClangArguments, project_path);
          time.ResetAndPrint("[perf] Loaded compilation entries (" + std::to_string(project->entries.size()) + " files)");

          // Start scanning include directories before dispatching project files, because that takes a long time.
          include_complete->Rescan();

          time.Reset();
          project->ForAllFilteredFiles(config, [&](int i, const Project::Entry& entry) {
            //std::cerr << "[" << i << "/" << (project->entries.size() - 1)
            //  << "] Dispatching index request for file " << entry.filename
            //  << std::endl;
            queue->index_request.Enqueue(Index_Request(entry.filename, entry.args));
          });

          // We need to support multiple concurrent index processes.
          time.ResetAndPrint("[perf] Dispatched initial index requests");
        }

        // TODO: query request->params.capabilities.textDocument and support only things
        // the client supports.

        auto response = Out_InitializeResponse();
        response.id = request->id;

        //response.result.capabilities.textDocumentSync = lsTextDocumentSyncOptions();
        //response.result.capabilities.textDocumentSync->openClose = true;
        //response.result.capabilities.textDocumentSync->change = lsTextDocumentSyncKind::Full;
        //response.result.capabilities.textDocumentSync->willSave = true;
        //response.result.capabilities.textDocumentSync->willSaveWaitUntil = true;
        response.result.capabilities.textDocumentSync = lsTextDocumentSyncKind::Incremental;

        response.result.capabilities.renameProvider = true;

        response.result.capabilities.completionProvider = lsCompletionOptions();
        response.result.capabilities.completionProvider->resolveProvider = false;
        // vscode doesn't support trigger character sequences, so we use ':' for '::' and '>' for '->'.
        // See https://github.com/Microsoft/language-server-protocol/issues/138.
        response.result.capabilities.completionProvider->triggerCharacters = { ".", ":", ">", "#" };

        response.result.capabilities.signatureHelpProvider = lsSignatureHelpOptions();
        // NOTE: If updating signature help tokens make sure to also update
        // WorkingFile::FindClosestCallNameInBuffer.
        response.result.capabilities.signatureHelpProvider->triggerCharacters = { "(", "," };

        response.result.capabilities.codeLensProvider = lsCodeLensOptions();
        response.result.capabilities.codeLensProvider->resolveProvider = false;

        response.result.capabilities.definitionProvider = true;
        response.result.capabilities.documentHighlightProvider = true;
        response.result.capabilities.hoverProvider = true;
        response.result.capabilities.referencesProvider = true;

        response.result.capabilities.codeActionProvider = true;

        response.result.capabilities.documentSymbolProvider = true;
        response.result.capabilities.workspaceSymbolProvider = true;

        response.result.capabilities.documentLinkProvider = lsDocumentLinkOptions();
        response.result.capabilities.documentLinkProvider->resolveProvider = false;

        ipc->SendOutMessageToClient(IpcId::Initialize, response);
        break;
      }

      case IpcId::Exit: {
        exit(0);
        break;
      }

      case IpcId::CqueryFreshenIndex: {
        LOG_S(INFO) << "Freshening " << project->entries.size() << " files";
        project->ForAllFilteredFiles(config, [&](int i, const Project::Entry& entry) {
          LOG_S(INFO) << "[" << i << "/" << (project->entries.size() - 1)
            << "] Dispatching index request for file " << entry.filename;
          queue->index_request.Enqueue(Index_Request(entry.filename, entry.args));
        });
        break;
      }

      case IpcId::CqueryTypeHierarchyTree: {
        auto msg = static_cast<Ipc_CqueryTypeHierarchyTree*>(message.get());

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_CqueryTypeHierarchyTree response;
        response.id = msg->id;

        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          if (ref.idx.kind == SymbolKind::Type) {
            response.result = BuildInheritanceHierarchyForType(db, working_files, QueryTypeId(ref.idx.idx));
            break;
          }
          if (ref.idx.kind == SymbolKind::Func) {
            response.result = BuildInheritanceHierarchyForFunc(db, working_files, QueryFuncId(ref.idx.idx));
            break;
          }
        }

        ipc->SendOutMessageToClient(IpcId::CqueryTypeHierarchyTree, response);
        break;
      }

      case IpcId::CqueryCallTreeInitial: {
        auto msg = static_cast<Ipc_CqueryCallTreeInitial*>(message.get());

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_CqueryCallTree response;
        response.id = msg->id;

        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          if (ref.idx.kind == SymbolKind::Func) {
            response.result = BuildInitialCallTree(db, working_files, QueryFuncId(ref.idx.idx));
            break;
          }
        }

        ipc->SendOutMessageToClient(IpcId::CqueryCallTreeInitial, response);
        break;
      }

      case IpcId::CqueryCallTreeExpand: {
        auto msg = static_cast<Ipc_CqueryCallTreeExpand*>(message.get());

        Out_CqueryCallTree response;
        response.id = msg->id;

        auto func_id = db->usr_to_func.find(msg->params.usr);
        if (func_id != db->usr_to_func.end())
          response.result = BuildExpandCallTree(db, working_files, func_id->second);

        ipc->SendOutMessageToClient(IpcId::CqueryCallTreeExpand, response);
        break;
      }

      case IpcId::CqueryVars: {
        auto msg = static_cast<Ipc_CqueryVars*>(message.get());

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_LocationList response;
        response.id = msg->id;
        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          if (ref.idx.kind == SymbolKind::Type) {
            optional<QueryType>& type = db->types[ref.idx.idx];
            if (!type) continue;
            std::vector<QueryLocation> locations = ToQueryLocation(db, type->instances);
            response.result = GetLsLocations(db, working_files, locations);
          }
        }
        ipc->SendOutMessageToClient(IpcId::CqueryVars, response);
        break;
      }

      case IpcId::CqueryCallers: {
        auto msg = static_cast<Ipc_CqueryCallers*>(message.get());

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_LocationList response;
        response.id = msg->id;
        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          if (ref.idx.kind == SymbolKind::Func) {
            optional<QueryFunc>& func = db->funcs[ref.idx.idx];
            if (!func) continue;
            std::vector<QueryLocation> locations = ToQueryLocation(db, func->callers);
            for (QueryFuncRef func_ref : GetCallersForAllBaseFunctions(db, *func))
              locations.push_back(func_ref.loc);
            for (QueryFuncRef func_ref : GetCallersForAllDerivedFunctions(db, *func))
              locations.push_back(func_ref.loc);

            response.result = GetLsLocations(db, working_files, locations);
          }
        }
        ipc->SendOutMessageToClient(IpcId::CqueryCallers, response);
        break;
      }

      case IpcId::CqueryBase: {
        auto msg = static_cast<Ipc_CqueryBase*>(message.get());

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_LocationList response;
        response.id = msg->id;
        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          if (ref.idx.kind == SymbolKind::Type) {
            optional<QueryType>& type = db->types[ref.idx.idx];
            if (!type) continue;
            std::vector<QueryLocation> locations = ToQueryLocation(db, type->def.parents);
            response.result = GetLsLocations(db, working_files, locations);
          }
          else if (ref.idx.kind == SymbolKind::Func) {
            optional<QueryFunc>& func = db->funcs[ref.idx.idx];
            if (!func) continue;
            optional<QueryLocation> location = GetBaseDefinitionOrDeclarationSpelling(db, *func);
            if (!location) continue;
            optional<lsLocation> ls_loc = GetLsLocation(db, working_files, *location);
            if (!ls_loc) continue;
            response.result.push_back(*ls_loc);
          }
        }
        ipc->SendOutMessageToClient(IpcId::CqueryBase, response);
        break;
      }

      case IpcId::CqueryDerived: {
        auto msg = static_cast<Ipc_CqueryDerived*>(message.get());

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_LocationList response;
        response.id = msg->id;
        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          if (ref.idx.kind == SymbolKind::Type) {
            optional<QueryType>& type = db->types[ref.idx.idx];
            if (!type) continue;
            std::vector<QueryLocation> locations = ToQueryLocation(db, type->derived);
            response.result = GetLsLocations(db, working_files, locations);
          }
          else if (ref.idx.kind == SymbolKind::Func) {
            optional<QueryFunc>& func = db->funcs[ref.idx.idx];
            if (!func) continue;
            std::vector<QueryLocation> locations = ToQueryLocation(db, func->derived);
            response.result = GetLsLocations(db, working_files, locations);
          }
        }
        ipc->SendOutMessageToClient(IpcId::CqueryDerived, response);
        break;
      }










      case IpcId::TextDocumentDidOpen: {
        // NOTE: This function blocks code lens. If it starts taking a long time
        // we will need to find a way to unblock the code lens request.

        Timer time;
        auto msg = static_cast<Ipc_TextDocumentDidOpen*>(message.get());
        std::string path = msg->params.textDocument.uri.GetPath();
        WorkingFile* working_file = working_files->OnOpen(msg->params);
        optional<std::string> cached_file_contents = LoadCachedFileContents(config, path);
        if (cached_file_contents)
          working_file->SetIndexContent(*cached_file_contents);
        else
          working_file->SetIndexContent(working_file->buffer_content);

        std::unique_ptr<IndexFile> cache = LoadCachedIndex(config, path);
        if (cache && !cache->skipped_by_preprocessor.empty())
          PublishInactiveLines(working_file, cache->skipped_by_preprocessor);

        time.ResetAndPrint("[querydb] Loading cached index file for DidOpen (blocks CodeLens)");

        include_complete->AddFile(working_file->filename);
        clang_complete->NotifyView(path);
        // TODO/FIXME
        //PriorityEnqueueFileForIndex(db, project, queue_do_index, working_file, path);

        break;
      }

      case IpcId::TextDocumentDidChange: {
        auto msg = static_cast<Ipc_TextDocumentDidChange*>(message.get());
        std::string path = msg->params.textDocument.uri.GetPath();
        working_files->OnChange(msg->params);
        clang_complete->NotifyEdit(path);
        break;
      }

      case IpcId::TextDocumentDidClose: {
        auto msg = static_cast<Ipc_TextDocumentDidClose*>(message.get());

        // Clear any diagnostics for the file.
        Out_TextDocumentPublishDiagnostics diag;
        diag.params.uri = msg->params.textDocument.uri;
        IpcManager::instance()->SendOutMessageToClient(IpcId::TextDocumentPublishDiagnostics, diag);

        // Remove internal state.
        working_files->OnClose(msg->params);

        break;
      }

      case IpcId::TextDocumentDidSave: {
        auto msg = static_cast<Ipc_TextDocumentDidSave*>(message.get());

        std::string path = msg->params.textDocument.uri.GetPath();
        // Send out an index request, and copy the current buffer state so we
        // can update the cached index contents when the index is done.
        //
        // We also do not index if there is already an index request.
        //
        // TODO: Cancel outgoing index request. Might be tricky to make
        //       efficient since we have to cancel.
        //    - we could have an |atomic<int> active_cancellations| variable
        //      that all of the indexers check before accepting an index. if
        //      zero we don't slow down fast-path. if non-zero we acquire
        //      mutex and check to see if we should skip the current request.
        //      if so, ignore that index response.
        // TODO: send as priority request
        Project::Entry entry = project->FindCompilationEntryForFile(path);
        queue->index_request.Enqueue(Index_Request(entry.filename, entry.args));

        clang_complete->NotifySave(path);

        break;
      }

      case IpcId::TextDocumentRename: {
        auto msg = static_cast<Ipc_TextDocumentRename*>(message.get());

        QueryFileId file_id;
        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file, &file_id))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_TextDocumentRename response;
        response.id = msg->id;

        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          // Found symbol. Return references to rename.
          std::vector<QueryLocation> uses = GetUsesOfSymbol(db, ref.idx);
          response.result = BuildWorkspaceEdit(db, working_files, uses, msg->params.newName);
          break;
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentRename, response);
        break;
      }

      case IpcId::TextDocumentCompletion: {
        auto msg = std::shared_ptr<Ipc_TextDocumentComplete>(
            static_cast<Ipc_TextDocumentComplete*>(message.release()));

        std::string path = msg->params.textDocument.uri.GetPath();
        WorkingFile* file = working_files->GetFileByFilename(path);

        // It shouldn't be possible, but sometimes vscode will send queries out
        // of order, ie, we get completion request before buffer content update.
        std::string buffer_line;
        if (msg->params.position.line >= 0 && msg->params.position.line < file->all_buffer_lines.size())
          buffer_line = file->all_buffer_lines[msg->params.position.line];

        if (ShouldRunIncludeCompletion(buffer_line)) {
          Out_TextDocumentComplete complete_response;
          complete_response.id = msg->id;
          complete_response.result.isIncomplete = false;

          {
            std::unique_lock<std::mutex> lock(include_complete->completion_items_mutex, std::defer_lock);
            if (include_complete->is_scanning)
              lock.lock();
            complete_response.result.items.assign(
              include_complete->completion_items.begin(),
              include_complete->completion_items.end());
            if (lock)
              lock.unlock();

            // Update textEdit params.
            for (lsCompletionItem& item : complete_response.result.items) {
              item.textEdit->range.start.line = msg->params.position.line;
              item.textEdit->range.start.character = 0;
              item.textEdit->range.end.line = msg->params.position.line;
              item.textEdit->range.end.character = (int)buffer_line.size();
            }
          }

          LOG_S(INFO) << "[complete] Returning " << complete_response.result.items.size() << " include completions";
          FilterCompletionResponse(&complete_response, buffer_line);
          ipc->SendOutMessageToClient(IpcId::TextDocumentCompletion, complete_response);
        }
        else {
          bool is_global_completion = false;
          std::string existing_completion;
          if (file) {
            msg->params.position = file->FindStableCompletionSource(msg->params.position, &is_global_completion, &existing_completion);
          }

          LOG_S(INFO) << "[complete] Got existing completion " << existing_completion;

          ClangCompleteManager::OnComplete callback = std::bind(
            [working_files, global_code_complete_cache, non_global_code_complete_cache, is_global_completion, existing_completion, msg]
            (const NonElidedVector<lsCompletionItem>& results, bool is_cached_result) {

            Out_TextDocumentComplete complete_response;
            complete_response.id = msg->id;
            complete_response.result.isIncomplete = false;
            complete_response.result.items = results;

            // Emit completion results.
            FilterCompletionResponse(&complete_response, existing_completion);
            IpcManager::instance()->SendOutMessageToClient(IpcId::TextDocumentCompletion, complete_response);

            // Cache completion results.
            if (!is_cached_result) {
              std::string path = msg->params.textDocument.uri.GetPath();
              if (is_global_completion) {
                global_code_complete_cache->WithLock([&]() {
                  global_code_complete_cache->cached_path_ = path;
                  LOG_S(INFO) << "[complete] Updating global_code_complete_cache->cached_results [0]";
                  global_code_complete_cache->cached_results_ = results;
                  LOG_S(INFO) << "[complete] DONE Updating global_code_complete_cache->cached_results [0]";
                });
              }
              else {
                non_global_code_complete_cache->WithLock([&]() {
                  non_global_code_complete_cache->cached_path_ = path;
                  non_global_code_complete_cache->cached_completion_position_ = msg->params.position;
                  LOG_S(INFO) << "[complete] Updating non_global_code_complete_cache->cached_results [1]";
                  non_global_code_complete_cache->cached_results_ = results;
                  LOG_S(INFO) << "[complete] DONE Updating non_global_code_complete_cache->cached_results [1]";
                });
              }
            }
          }, std::placeholders::_1, std::placeholders::_2);

          bool is_cache_match = false;
          global_code_complete_cache->WithLock([&]() {
            is_cache_match = is_global_completion && global_code_complete_cache->cached_path_ == path && !global_code_complete_cache->cached_results_.empty();
          });
          if (is_cache_match) {
            LOG_S(INFO) << "[complete] Early-returning cached global completion results at " << msg->params.position.ToString();

            ClangCompleteManager::OnComplete freshen_global =
              [global_code_complete_cache]
              (NonElidedVector<lsCompletionItem> results, bool is_cached_result) {

              assert(!is_cached_result);

              LOG_S(INFO) << "[complete] Updating global_code_complete_cache->cached_results [2]";
              // note: path is updated in the normal completion handler.
              global_code_complete_cache->WithLock([&]() {
                global_code_complete_cache->cached_results_ = results;
              });
              LOG_S(INFO) << "[complete] DONE Updating global_code_complete_cache->cached_results [2]";
            };

            global_code_complete_cache->WithLock([&]() {
              callback(global_code_complete_cache->cached_results_, true /*is_cached_result*/);
            });
            clang_complete->CodeComplete(msg->params, freshen_global);
          }
          else if (non_global_code_complete_cache->IsCacheValid(msg->params)) {
            LOG_S(INFO) << "[complete] Using cached completion results at " << msg->params.position.ToString();
            non_global_code_complete_cache->WithLock([&]() {
              callback(non_global_code_complete_cache->cached_results_, true /*is_cached_result*/);
            });
          }
          else {
            clang_complete->CodeComplete(msg->params, callback);
          }
        }

        break;
      }

      case IpcId::TextDocumentSignatureHelp: {
        auto msg = static_cast<Ipc_TextDocumentSignatureHelp*>(message.get());
        lsTextDocumentPositionParams& params = msg->params;
        WorkingFile* file = working_files->GetFileByFilename(params.textDocument.uri.GetPath());
        std::string search;
        int active_param = 0;
        if (file) {
          lsPosition completion_position;
          search = file->FindClosestCallNameInBuffer(params.position, &active_param, &completion_position);
          params.position = completion_position;
        }
        //LOG_S(INFO) << "[completion] Returning signatures for " << search;
        if (search.empty())
          break;

        ClangCompleteManager::OnComplete callback = std::bind(
          [signature_cache]
          (BaseIpcMessage* message, std::string search, int active_param, const NonElidedVector<lsCompletionItem>& results, bool is_cached_result) {
          auto msg = static_cast<Ipc_TextDocumentSignatureHelp*>(message);
          auto ipc = IpcManager::instance();

          Out_TextDocumentSignatureHelp response;
          response.id = msg->id;

          for (auto& result : results) {
            if (result.label != search)
              continue;

            lsSignatureInformation signature;
            signature.label = result.detail;
            for (auto& parameter : result.parameters_) {
              lsParameterInformation ls_param;
              ls_param.label = parameter;
              signature.parameters.push_back(ls_param);
            }
            response.result.signatures.push_back(signature);
          }

          // Guess the signature the user wants based on available parameter
          // count.
          response.result.activeSignature = 0;
          for (size_t i = 0; i < response.result.signatures.size(); ++i) {
            if (active_param < response.result.signatures.size()) {
              response.result.activeSignature = (int)i;
              break;
            }
          }

          // Set signature to what we parsed from the working file.
          response.result.activeParameter = active_param;

          Timer timer;
          ipc->SendOutMessageToClient(IpcId::TextDocumentSignatureHelp, response);
          timer.ResetAndPrint("[complete] Writing signature help results");

          if (!is_cached_result) {
            signature_cache->WithLock([&]() {
              signature_cache->cached_path_ = msg->params.textDocument.uri.GetPath();
              signature_cache->cached_completion_position_ = msg->params.position;
              LOG_S(INFO) << "[complete] Updating signature_cache->cached_results [3]";
              signature_cache->cached_results_ = results;
            });
          }

          delete message;
        }, message.release(), search, active_param, std::placeholders::_1, std::placeholders::_2);

        if (signature_cache->IsCacheValid(params)) {
          LOG_S(INFO) << "[complete] Using cached completion results at " << params.position.ToString();
          signature_cache->WithLock([&]() {
            callback(signature_cache->cached_results_, true /*is_cached_result*/);
          });
        }
        else {
          clang_complete->CodeComplete(params, std::move(callback));
        }

        break;
      }

      case IpcId::TextDocumentDefinition: {
        auto msg = static_cast<Ipc_TextDocumentDefinition*>(message.get());

        QueryFileId file_id;
        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file, &file_id))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_TextDocumentDefinition response;
        response.id = msg->id;

        int target_line = msg->params.position.line + 1;
        int target_column = msg->params.position.character + 1;

        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          // Found symbol. Return definition.

          // Special cases which are handled:
          //  - symbol has declaration but no definition (ie, pure virtual)
          //  - start at spelling but end at extent for better mouse tooltip
          //  - goto declaration while in definition of recursive type

          optional<QueryLocation> def_loc = GetDefinitionSpellingOfSymbol(db, ref.idx);

          // We use spelling start and extent end because this causes vscode to
          // highlight the entire definition when previewing / hoving with the
          // mouse.
          optional<QueryLocation> def_extent = GetDefinitionExtentOfSymbol(db, ref.idx);
          if (def_loc && def_extent)
            def_loc->range.end = def_extent->range.end;

          // If the cursor is currently at or in the definition we should goto
          // the declaration if possible. We also want to use declarations if
          // we're pointing to, ie, a pure virtual function which has no
          // definition.
          if (!def_loc || (def_loc->path == file_id &&
                            def_loc->range.Contains(target_line, target_column))) {
            // Goto declaration.

            std::vector<QueryLocation> declarations = GetDeclarationsOfSymbolForGotoDefinition(db, ref.idx);
            for (auto declaration : declarations) {
              optional<lsLocation> ls_declaration = GetLsLocation(db, working_files, declaration);
              if (ls_declaration)
                response.result.push_back(*ls_declaration);
            }
            // We found some declarations. Break so we don't add the definition location.
            if (!response.result.empty())
              break;
          }

          if (def_loc)
            PushBack(&response.result, GetLsLocation(db, working_files, *def_loc));

          if (!response.result.empty())
            break;
        }

        // No symbols - check for includes.
        if (response.result.empty()) {
          for (const IndexInclude& include : file->def.includes) {
            if (include.line == target_line) {
              lsLocation result;
              result.uri = lsDocumentUri::FromPath(include.resolved_path);
              response.result.push_back(result);
              break;
            }
          }
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentDefinition, response);
        break;
      }

      case IpcId::TextDocumentDocumentHighlight: {
        auto msg = static_cast<Ipc_TextDocumentDocumentHighlight*>(message.get());

        QueryFileId file_id;
        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file, &file_id))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_TextDocumentDocumentHighlight response;
        response.id = msg->id;

        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          // Found symbol. Return references to highlight.
          std::vector<QueryLocation> uses = GetUsesOfSymbol(db, ref.idx);
          response.result.reserve(uses.size());
          for (const QueryLocation& use : uses) {
            if (use.path != file_id)
              continue;

            optional<lsLocation> ls_location = GetLsLocation(db, working_files, use);
            if (!ls_location)
              continue;

            lsDocumentHighlight highlight;
            highlight.kind = lsDocumentHighlightKind::Text;
            highlight.range = ls_location->range;
            response.result.push_back(highlight);
          }
          break;
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentDocumentHighlight, response);
        break;
      }

      case IpcId::TextDocumentHover: {
        auto msg = static_cast<Ipc_TextDocumentHover*>(message.get());

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_TextDocumentHover response;
        response.id = msg->id;

        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          // Found symbol. Return hover.
          optional<lsRange> ls_range = GetLsRange(working_files->GetFileByFilename(file->def.path), ref.loc.range);
          if (!ls_range)
            continue;

          response.result.contents = GetHoverForSymbol(db, ref.idx);
          response.result.range = *ls_range;
          break;
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentHover, response);
        break;
      }

      case IpcId::TextDocumentReferences: {
        auto msg = static_cast<Ipc_TextDocumentReferences*>(message.get());

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(file->def.path);

        Out_TextDocumentReferences response;
        response.id = msg->id;

        for (const SymbolRef& ref : FindSymbolsAtLocation(working_file, file, msg->params.position)) {
          optional<QueryLocation> excluded_declaration;
          if (!msg->params.context.includeDeclaration) {
            LOG_S(INFO) << "Excluding declaration in references";
            excluded_declaration = GetDefinitionSpellingOfSymbol(db, ref.idx);
          }

          // Found symbol. Return references.
          std::vector<QueryLocation> uses = GetUsesOfSymbol(db, ref.idx);
          response.result.reserve(uses.size());
          for (const QueryLocation& use : uses) {
            if (excluded_declaration.has_value() && use == *excluded_declaration)
              continue;

            optional<lsLocation> ls_location = GetLsLocation(db, working_files, use);
            if (ls_location)
              response.result.push_back(*ls_location);
          }
          break;
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentReferences, response);
        break;
      }

      case IpcId::TextDocumentDocumentSymbol: {
        auto msg = static_cast<Ipc_TextDocumentDocumentSymbol*>(message.get());

        Out_TextDocumentDocumentSymbol response;
        response.id = msg->id;

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;


        for (SymbolRef ref : file->def.outline) {
          optional<lsSymbolInformation> info = GetSymbolInfo(db, working_files, ref.idx);
          if (!info)
            continue;

          optional<lsLocation> location = GetLsLocation(db, working_files, ref.loc);
          if (!location)
            continue;
          info->location = *location;
          response.result.push_back(*info);
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentDocumentSymbol, response);
        break;
      }

      case IpcId::TextDocumentDocumentLink: {
        auto msg = static_cast<Ipc_TextDocumentDocumentLink*>(message.get());

        Out_TextDocumentDocumentLink response;
        response.id = msg->id;

        if (config->showDocumentLinksOnIncludes) {
          QueryFile* file;
          if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
            break;

          WorkingFile* working_file = working_files->GetFileByFilename(msg->params.textDocument.uri.GetPath());
          if (!working_file) {
            LOG_S(INFO) << "Unable to find working file " << msg->params.textDocument.uri.GetPath();
            break;
          }
          for (const IndexInclude& include : file->def.includes) {
            optional<int> buffer_line;
            optional<std::string> buffer_line_content = working_file->GetBufferLineContentFromIndexLine(include.line, &buffer_line);
            if (!buffer_line || !buffer_line_content)
              continue;

            // Subtract 1 from line because querydb stores 1-based lines but
            // vscode expects 0-based lines.
            optional<lsRange> between_quotes = ExtractQuotedRange(*buffer_line - 1, *buffer_line_content);
            if (!between_quotes)
              continue;

            lsDocumentLink link;
            link.target = lsDocumentUri::FromPath(include.resolved_path);
            link.range = *between_quotes;
            response.result.push_back(link);
          }
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentDocumentLink, response);
        break;
      }

      case IpcId::TextDocumentCodeAction: {
        // NOTE: This code snippet will generate some FixIts for testing:
        //
        //    struct origin { int x, int y };
        //    void foo() {
        //      point origin = {
        //        x: 0.0,
        //        y: 0.0
        //      };
        //    }
        //
        auto msg = static_cast<Ipc_TextDocumentCodeAction*>(message.get());

        QueryFileId file_id;
        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file, &file_id))
          break;

        WorkingFile* working_file = working_files->GetFileByFilename(msg->params.textDocument.uri.GetPath());
        if (!working_file) {
          // TODO: send error response.
          LOG_S(INFO) << "[error] textDocument/codeAction could not find working file";
          break;
        }

        Out_TextDocumentCodeAction response;
        response.id = msg->id;

        // TODO: auto-insert namespace?

        int default_line = (int)working_file->all_buffer_lines.size();

        // Make sure to call EnsureImplFile before using these. We lazy load
        // them because computing the values could involve an entire project
        // scan.
        optional<lsDocumentUri> impl_uri;
        optional<QueryFileId> impl_file_id;

        std::vector<SymbolRef> syms = FindSymbolsAtLocation(working_file, file, msg->params.range.start);
        for (SymbolRef sym : syms) {
          switch (sym.idx.kind) {
            case SymbolKind::Type: {
              optional<QueryType>& type = db->types[sym.idx.idx];
              if (!type)
                break;

              int num_edits = 0;

              // Get implementation file.
              Out_TextDocumentCodeAction::Command command;

              for (QueryFuncId func_id : type->def.funcs) {
                optional<QueryFunc>& func_def = db->funcs[func_id.id];
                if (!func_def || func_def->def.definition_extent)
                  continue;

                EnsureImplFile(db, file_id, impl_uri /*out*/, impl_file_id /*out*/);
                optional<lsTextEdit> edit = BuildAutoImplementForFunction(db, working_files, working_file, default_line, file_id, *impl_file_id, *func_def);
                if (!edit)
                  continue;

                ++num_edits;

                // Merge edits together if they are on the same line.
                // TODO: be smarter about newline merging? ie, don't end up
                //       with foo()\n\n\n\nfoo(), we want foo()\n\nfoo()\n\n
                //
                if (!command.arguments.edits.empty() &&
                    command.arguments.edits[command.arguments.edits.size() - 1].range.end.line == edit->range.start.line) {
                  command.arguments.edits[command.arguments.edits.size() - 1].newText += edit->newText;
                }
                else {
                  command.arguments.edits.push_back(*edit);
                }
              }
              if (command.arguments.edits.empty())
                break;

              // If we're inserting at the end of the document, put a newline before the insertion.
              if (command.arguments.edits[0].range.start.line >= default_line)
                command.arguments.edits[0].newText.insert(0, "\n");

              command.arguments.textDocumentUri = *impl_uri;
              command.title = "Auto-Implement " + std::to_string(num_edits) + " methods on " + type->def.short_name;
              command.command = "cquery._autoImplement";
              response.result.push_back(command);
              break;
            }

            case SymbolKind::Func: {
              optional<QueryFunc>& func = db->funcs[sym.idx.idx];
              if (!func || func->def.definition_extent)
                break;

              EnsureImplFile(db, file_id, impl_uri /*out*/, impl_file_id /*out*/);

              // Get implementation file.
              Out_TextDocumentCodeAction::Command command;
              command.title = "Auto-Implement " + func->def.short_name;
              command.command = "cquery._autoImplement";
              command.arguments.textDocumentUri = *impl_uri;
              optional<lsTextEdit> edit = BuildAutoImplementForFunction(db, working_files, working_file, default_line, file_id, *impl_file_id, *func);
              if (!edit)
                break;

              // If we're inserting at the end of the document, put a newline before the insertion.
              if (edit->range.start.line >= default_line)
                edit->newText.insert(0, "\n");
              command.arguments.edits.push_back(*edit);
              response.result.push_back(command);
              break;
            }
            default:
              break;
          }

          // Only show one auto-impl section.
          if (!response.result.empty())
            break;
        }

        std::vector<lsDiagnostic> diagnostics;
        working_files->DoAction([&]() {
          diagnostics = working_file->diagnostics_;
        });
        for (lsDiagnostic& diag : diagnostics) {
          if (diag.range.start.line != msg->params.range.start.line)
            continue;

          // For error diagnostics, provide an action to resolve an include.
          // TODO: find a way to index diagnostic contents so line numbers
          // don't get mismatched when actively editing a file.
          std::string include_query = LexWordAroundPos(diag.range.start, working_file->buffer_content);
          if (diag.severity == lsDiagnosticSeverity::Error && !include_query.empty()) {
            const size_t kMaxResults = 20;


            std::unordered_set<std::string> include_absolute_paths;

            // Find include candidate strings.
            for (int i = 0; i < db->detailed_names.size(); ++i) {
              if (include_absolute_paths.size() > kMaxResults)
                break;
              if (db->detailed_names[i].find(include_query) == std::string::npos)
                continue;

              optional<QueryFileId> decl_file_id = GetDeclarationFileForSymbol(db, db->symbols[i]);
              if (!decl_file_id)
                continue;

              optional<QueryFile>& decl_file = db->files[decl_file_id->id];
              if (!decl_file)
                continue;

              include_absolute_paths.insert(decl_file->def.path);
            }

            // Build include strings.
            std::unordered_set<std::string> include_insert_strings;
            include_insert_strings.reserve(include_absolute_paths.size());

            for (const std::string& path : include_absolute_paths) {
              optional<lsCompletionItem> item = include_complete->FindCompletionItemForAbsolutePath(path);
              if (!item)
                continue;
              if (item->textEdit)
                include_insert_strings.insert(item->textEdit->newText);
              else if (!item->insertText.empty())
                include_insert_strings.insert(item->insertText);
              else
                assert(false && "unable to determine insert string for include completion item");
            }

            // Build code action.
            if (!include_insert_strings.empty()) {
              Out_TextDocumentCodeAction::Command command;

              // Build edits.
              for (const std::string& include_insert_string : include_insert_strings) {
                lsTextEdit edit;
                optional<int> include_line = FindIncludeLine(working_file->all_buffer_lines, include_insert_string);
                if (!include_line)
                  continue;

                edit.range.start.line = *include_line;
                edit.range.end.line = *include_line;
                edit.newText = include_insert_string + "\n";
                command.arguments.edits.push_back(edit);
              }

              // Setup metadata and send to client.
              if (include_insert_strings.size() == 1)
                command.title = "Insert " + *include_insert_strings.begin();
              else
                command.title = "Pick one of " + std::to_string(command.arguments.edits.size()) + " includes to insert";
              command.command = "cquery._insertInclude";
              command.arguments.textDocumentUri = msg->params.textDocument.uri;
              response.result.push_back(command);
            }
          }

          // clang does not provide accurate enough column reporting for
          // diagnostics to do good column filtering, so report all
          // diagnostics on the line.
          if (!diag.fixits_.empty()) {
            Out_TextDocumentCodeAction::Command command;
            command.title = "FixIt: " + diag.message;
            command.command = "cquery._applyFixIt";
            command.arguments.textDocumentUri = msg->params.textDocument.uri;
            command.arguments.edits = diag.fixits_;
            response.result.push_back(command);
          }
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentCodeAction, response);
        break;
      }

      case IpcId::TextDocumentCodeLens: {
        auto msg = static_cast<Ipc_TextDocumentCodeLens*>(message.get());

        Out_TextDocumentCodeLens response;
        response.id = msg->id;

        lsDocumentUri file_as_uri = msg->params.textDocument.uri;
        std::string path = file_as_uri.GetPath();

        clang_complete->NotifyView(path);

        QueryFile* file;
        if (!FindFileOrFail(db, msg->id, msg->params.textDocument.uri.GetPath(), &file))
          break;

        CommonCodeLensParams common;
        common.result = &response.result;
        common.db = db;
        common.working_files = working_files;
        common.working_file = working_files->GetFileByFilename(file->def.path);

        for (SymbolRef ref : file->def.outline) {
          // NOTE: We OffsetColumn so that the code lens always show up in a
          // predictable order. Otherwise, the client may randomize it.

          SymbolIdx symbol = ref.idx;
          switch (symbol.kind) {
          case SymbolKind::Type: {
            optional<QueryType>& type = db->types[symbol.idx];
            if (!type)
              continue;
            AddCodeLens("ref", "refs", &common, ref.loc.OffsetStartColumn(0), type->uses, type->def.definition_spelling, true /*force_display*/);
            AddCodeLens("derived", "derived", &common, ref.loc.OffsetStartColumn(1), ToQueryLocation(db, type->derived), nullopt, false /*force_display*/);
            AddCodeLens("var", "vars", &common, ref.loc.OffsetStartColumn(2), ToQueryLocation(db, type->instances), nullopt, false /*force_display*/);
            break;
          }
          case SymbolKind::Func: {
            optional<QueryFunc>& func = db->funcs[symbol.idx];
            if (!func)
              continue;

            int16_t offset = 0;

            std::vector<QueryFuncRef> base_callers = GetCallersForAllBaseFunctions(db, *func);
            std::vector<QueryFuncRef> derived_callers = GetCallersForAllDerivedFunctions(db, *func);
            if (base_callers.empty() && derived_callers.empty()) {
              AddCodeLens("call", "calls", &common, ref.loc.OffsetStartColumn(offset++), ToQueryLocation(db, func->callers), nullopt, true /*force_display*/);
            }
            else {
              AddCodeLens("direct call", "direct calls", &common, ref.loc.OffsetStartColumn(offset++), ToQueryLocation(db, func->callers), nullopt, false /*force_display*/);
              if (!base_callers.empty())
                AddCodeLens("base call", "base calls", &common, ref.loc.OffsetStartColumn(offset++), ToQueryLocation(db, base_callers), nullopt, false /*force_display*/);
              if (!derived_callers.empty())
                AddCodeLens("derived call", "derived calls", &common, ref.loc.OffsetStartColumn(offset++), ToQueryLocation(db, derived_callers), nullopt, false /*force_display*/);
            }

            AddCodeLens("derived", "derived", &common, ref.loc.OffsetStartColumn(offset++), ToQueryLocation(db, func->derived), nullopt, false /*force_display*/);

            // "Base"
            optional<QueryLocation> base_loc = GetBaseDefinitionOrDeclarationSpelling(db, *func);
            if (base_loc) {
              optional<lsLocation> ls_base = GetLsLocation(db, working_files, *base_loc);
              if (ls_base) {
                optional<lsRange> range = GetLsRange(common.working_file, ref.loc.range);
                if (range) {
                  TCodeLens code_lens;
                  code_lens.range = *range;
                  code_lens.range.start.character += offset++;
                  code_lens.command = lsCommand<lsCodeLensCommandArguments>();
                  code_lens.command->title = "Base";
                  code_lens.command->command = "cquery.goto";
                  code_lens.command->arguments.uri = ls_base->uri;
                  code_lens.command->arguments.position = ls_base->range.start;
                  response.result.push_back(code_lens);
                }
              }
            }

            break;
          }
          case SymbolKind::Var: {
            optional<QueryVar>& var = db->vars[symbol.idx];
            if (!var)
              continue;

            if (var->def.is_local && !config->codeLensOnLocalVariables)
              continue;

            bool force_display = true;
            // Do not show 0 refs on macro with no uses, as it is most likely a
            // header guard.
            if (var->def.is_macro)
              force_display = false;

            AddCodeLens("ref", "refs", &common, ref.loc.OffsetStartColumn(0), var->uses, var->def.definition_spelling, force_display);
            break;
          }
          case SymbolKind::File:
          case SymbolKind::Invalid: {
            assert(false && "unexpected");
            break;
          }
          };
        }

        ipc->SendOutMessageToClient(IpcId::TextDocumentCodeLens, response);
        break;
      }

      case IpcId::WorkspaceSymbol: {
        // TODO: implement fuzzy search, see https://github.com/junegunn/fzf/blob/master/src/matcher.go for inspiration
        auto msg = static_cast<Ipc_WorkspaceSymbol*>(message.get());

        Out_WorkspaceSymbol response;
        response.id = msg->id;


        LOG_S(INFO) << "[querydb] Considering " << db->detailed_names.size()
          << " candidates for query " << msg->params.query;

        std::string query = msg->params.query;

        std::unordered_set<std::string> inserted_results;
        inserted_results.reserve(config->maxWorkspaceSearchResults);

        for (int i = 0; i < db->detailed_names.size(); ++i) {
          if (db->detailed_names[i].find(query) != std::string::npos) {
            // Do not show the same entry twice.
            if (!inserted_results.insert(db->detailed_names[i]).second)
              continue;

            InsertSymbolIntoResult(db, working_files, db->symbols[i], &response.result);
            if (response.result.size() >= config->maxWorkspaceSearchResults)
              break;
          }
        }

        if (response.result.size() < config->maxWorkspaceSearchResults) {
          for (int i = 0; i < db->detailed_names.size(); ++i) {
            if (SubstringMatch(query, db->detailed_names[i])) {
              // Do not show the same entry twice.
              if (!inserted_results.insert(db->detailed_names[i]).second)
                continue;

              InsertSymbolIntoResult(db, working_files, db->symbols[i], &response.result);
              if (response.result.size() >= config->maxWorkspaceSearchResults)
                break;
            }
          }
        }

        LOG_S(INFO) << "[querydb] Found " << response.result.size() << " results for query " << query;
        ipc->SendOutMessageToClient(IpcId::WorkspaceSymbol, response);
        break;
      }

      default: {
        LOG_S(INFO) << "[querydb] Unhandled IPC message " << IpcIdToString(message->method_id);
        exit(1);
      }
    }
  }

  // TODO: consider rate-limiting and checking for IPC messages so we don't block
  // requests / we can serve partial requests.

  if (QueryDb_ImportMain(config, db, queue, working_files))
    did_work = true;

  return did_work;
}

void QueryDbMain(const std::string& bin_name, Config* config, MultiQueueWaiter* waiter) {
  // Create queues.
  QueueManager queue(waiter);

  Project project;
  WorkingFiles working_files;
  ClangCompleteManager clang_complete(
    config, &project, &working_files,
    std::bind(&EmitDiagnostics, &working_files, std::placeholders::_1, std::placeholders::_2));
  IncludeComplete include_complete(config, &project);
  auto global_code_complete_cache = MakeUnique<CodeCompleteCache>();
  auto non_global_code_complete_cache = MakeUnique<CodeCompleteCache>();
  auto signature_cache = MakeUnique<CodeCompleteCache>();
  FileConsumer::SharedState file_consumer_shared;

  // Run query db main loop.
  SetCurrentThreadName("querydb");
  QueryDatabase db;
  while (true) {
    bool did_work = QueryDbMainLoop(
      config, &db, waiter, &queue,
      &project, &file_consumer_shared, &working_files,
      &clang_complete, &include_complete, global_code_complete_cache.get(), non_global_code_complete_cache.get(), signature_cache.get());
    if (!did_work) {
      waiter->Wait({
        IpcManager::instance()->threaded_queue_for_server_.get(),
        &queue.do_id_map,
        &queue.on_indexed
      });
    }
  }
}































































// TODO: global lock on stderr output.

// Separate thread whose only job is to read from stdin and
// dispatch read commands to the actual indexer program. This
// cannot be done on the main thread because reading from std::cin
// blocks.
//
// |ipc| is connected to a server.
void LanguageServerStdinLoop(Config* config, std::unordered_map<IpcId, Timer>* request_times) {
  IpcManager* ipc = IpcManager::instance();

  SetCurrentThreadName("stdin");
  while (true) {
    std::unique_ptr<BaseIpcMessage> message = MessageRegistry::instance()->ReadMessageFromStdin();

    // Message parsing can fail if we don't recognize the method.
    if (!message)
      continue;

    (*request_times)[message->method_id] = Timer();

    //std::cerr << "[stdin] Got message " << IpcIdToString(message->method_id) << std::endl;
    switch (message->method_id) {
    case IpcId::Initialized: {
      // TODO: don't send output until we get this notification
      break;
    }

    case IpcId::CancelRequest: {
      // TODO: support cancellation
      break;
    }

    case IpcId::Initialize:
    case IpcId::Exit:
    case IpcId::TextDocumentDidOpen:
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
    case IpcId::CqueryDerived: {
      ipc->SendMessage(IpcManager::Destination::Server, std::move(message));
      break;
    }

    default: {
      std::cerr << "[stdin] Unhandled IPC message " << IpcIdToString(message->method_id) << std::endl;
      exit(1);
    }
    }
  }
}














































void StdoutMain(std::unordered_map<IpcId, Timer>* request_times, MultiQueueWaiter* waiter) {
  SetCurrentThreadName("stdout");
  IpcManager* ipc = IpcManager::instance();

  while (true) {
    std::vector<std::unique_ptr<BaseIpcMessage>> messages = ipc->GetMessages(IpcManager::Destination::Client);
    if (messages.empty()) {
      waiter->Wait({ipc->threaded_queue_for_client_.get()});
      continue;
    }

    for (auto& message : messages) {
      //std::cerr << "[stdout] Processing message " << IpcIdToString(message->method_id) << std::endl;

      switch (message->method_id) {
        case IpcId::Cout: {
          auto msg = static_cast<Ipc_Cout*>(message.get());

          if (ShouldDisplayIpcTiming(msg->original_ipc_id)) {
            Timer time = (*request_times)[msg->original_ipc_id];
            time.ResetAndPrint("[e2e] Running " + std::string(IpcIdToString(msg->original_ipc_id)));
          }

          std::cout << msg->content;
          std::cout.flush();
          break;
        }

        default: {
          std::cerr << "[stdout] Unhandled IPC message " << IpcIdToString(message->method_id) << std::endl;
          exit(1);
        }
      }
    }
  }
}

void LanguageServerMain(const std::string& bin_name, Config* config, MultiQueueWaiter* waiter) {
  std::unordered_map<IpcId, Timer> request_times;

  // Start stdin reader. Reading from stdin is a blocking operation so this
  // needs a dedicated thread.
  new std::thread([&]() {
    LanguageServerStdinLoop(config, &request_times);
  });

  // Start querydb thread. querydb will start indexer threads as needed.
  new std::thread([&]() {
    QueryDbMain(bin_name, config, waiter);
  });

  // We run a dedicated thread for writing to stdout because there can be an
  // unknown number of delays when output information.
  StdoutMain(&request_times, waiter);
}


















































// What's the plan?
//
// - start up a separate process for indexing
// - send file paths to it to index raw compile_commands.json, it can reparse as needed
// -

int main(int argc, char** argv) {
  loguru::init(argc, argv);
  loguru::g_flush_interval_ms = 0;

  MultiQueueWaiter waiter;
  IpcManager::CreateInstance(&waiter);

  //bool loop = true;
  //while (loop)
  //  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  //std::this_thread::sleep_for(std::chrono::seconds(10));

  PlatformInit();
  IndexInit();

  RegisterMessageTypes();

  std::unordered_map<std::string, std::string> options =
    ParseOptions(argc, argv);

  if (HasOption(options, "--test")) {
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    int res = context.run();
    if (context.shouldExit())
      return res;

    for (int i = 0; i < 1; ++i)
      RunTests();

    /*
    for (int i = 0; i < 1; ++i) {
      std::this_thread::sleep_for(std::chrono::seconds(5));
      std::cerr << "[POST] Memory usage: " << GetProcessMemoryUsedInMb() << "mb" << std::endl;
    }
    */

    std::cerr << std::endl << "[Enter] to exit" << std::endl;
    std::cin.get();
    return 0;
  }
  else if (HasOption(options, "--language-server")) {
    //std::cerr << "Running language server" << std::endl;
    Config config;
    LanguageServerMain(argv[0], &config, &waiter);
    return 0;
  }
  else {
    std::cout << R"help(cquery help:

  cquery is a low-latency C++ language server.

  General:
    --help        Print this help information.
    --language-server
                  Run as a language server. This implements the language
                  server spec over STDIN and STDOUT.
    --test        Run tests. Does nothing if test support is not compiled in.

  Configuration:
    When opening up a directory, cquery will look for a compile_commands.json
    file emitted by your preferred build system. If not present, cquery will
    use a recursive directory listing instead. Command line flags can be
    provided by adding a "clang_args" file in the top-level directory. Each
    line in that file is a separate argument.

    There are also a number of configuration options available when
    initializing the language server - your editor should have tooling to
    describe those options. See |Config| in this source code for a detailed
    list of all currently supported options.
)help";
    return 0;
  }
}



TEST_SUITE("LexFunctionDeclaration");

TEST_CASE("simple") {
  std::string buffer_content = " void Foo(); ";
  lsPosition declaration = CharPos(buffer_content, 'F');
  std::string insert_text;
  int newlines_after_name = 0;

  LexFunctionDeclaration(buffer_content, declaration, nullopt, &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "void Foo() {\n}");
  REQUIRE(newlines_after_name == 0);

  LexFunctionDeclaration(buffer_content, declaration, std::string("Type"), &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "void Type::Foo() {\n}");
  REQUIRE(newlines_after_name == 0);
}

TEST_CASE("ctor") {
  std::string buffer_content = " Foo(); ";
  lsPosition declaration = CharPos(buffer_content, 'F');
  std::string insert_text;
  int newlines_after_name = 0;

  LexFunctionDeclaration(buffer_content, declaration, std::string("Foo"), &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "Foo::Foo() {\n}");
  REQUIRE(newlines_after_name == 0);
}

TEST_CASE("dtor") {
  std::string buffer_content = " ~Foo(); ";
  lsPosition declaration = CharPos(buffer_content, '~');
  std::string insert_text;
  int newlines_after_name = 0;

  LexFunctionDeclaration(buffer_content, declaration, std::string("Foo"), &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "Foo::~Foo() {\n}");
  REQUIRE(newlines_after_name == 0);
}

TEST_CASE("complex return type") {
  std::string buffer_content = " std::vector<int> Foo(); ";
  lsPosition declaration = CharPos(buffer_content, 'F');
  std::string insert_text;
  int newlines_after_name = 0;

  LexFunctionDeclaration(buffer_content, declaration, nullopt, &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "std::vector<int> Foo() {\n}");
  REQUIRE(newlines_after_name == 0);

  LexFunctionDeclaration(buffer_content, declaration, std::string("Type"), &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "std::vector<int> Type::Foo() {\n}");
  REQUIRE(newlines_after_name == 0);
}

TEST_CASE("extra complex return type") {
  std::string buffer_content = " std::function < int() > \n Foo(); ";
  lsPosition declaration = CharPos(buffer_content, 'F');
  std::string insert_text;
  int newlines_after_name = 0;

  LexFunctionDeclaration(buffer_content, declaration, nullopt, &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "std::function < int() > \n Foo() {\n}");
  REQUIRE(newlines_after_name == 0);

  LexFunctionDeclaration(buffer_content, declaration, std::string("Type"), &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "std::function < int() > \n Type::Foo() {\n}");
  REQUIRE(newlines_after_name == 0);
}

TEST_CASE("parameters") {
  std::string buffer_content = "void Foo(int a,\n\n    int b); ";
  lsPosition declaration = CharPos(buffer_content, 'F');
  std::string insert_text;
  int newlines_after_name = 0;

  LexFunctionDeclaration(buffer_content, declaration, nullopt, &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "void Foo(int a,\n\n    int b) {\n}");
  REQUIRE(newlines_after_name == 2);

  LexFunctionDeclaration(buffer_content, declaration, std::string("Type"), &insert_text, &newlines_after_name);
  REQUIRE(insert_text == "void Type::Foo(int a,\n\n    int b) {\n}");
  REQUIRE(newlines_after_name == 2);
}

TEST_SUITE_END();



TEST_SUITE("LexWordAroundPos");

TEST_CASE("edges") {
  std::string content = "Foobar";
  REQUIRE(LexWordAroundPos(CharPos(content, 'F'), content) == "Foobar");
  REQUIRE(LexWordAroundPos(CharPos(content, 'o'), content) == "Foobar");
  REQUIRE(LexWordAroundPos(CharPos(content, 'b'), content) == "Foobar");
  REQUIRE(LexWordAroundPos(CharPos(content, 'a'), content) == "Foobar");
  REQUIRE(LexWordAroundPos(CharPos(content, 'r'), content) == "Foobar");
}

TEST_CASE("simple") {
  std::string content = "  Foobar  ";
  REQUIRE(LexWordAroundPos(CharPos(content, 'F'), content) == "Foobar");
  REQUIRE(LexWordAroundPos(CharPos(content, 'o'), content) == "Foobar");
  REQUIRE(LexWordAroundPos(CharPos(content, 'b'), content) == "Foobar");
  REQUIRE(LexWordAroundPos(CharPos(content, 'a'), content) == "Foobar");
  REQUIRE(LexWordAroundPos(CharPos(content, 'r'), content) == "Foobar");
}

TEST_CASE("underscores and numbers") {
  std::string content = "  _my_t5ype7  ";
  REQUIRE(LexWordAroundPos(CharPos(content, '_'), content) == "_my_t5ype7");
  REQUIRE(LexWordAroundPos(CharPos(content, '5'), content) == "_my_t5ype7");
  REQUIRE(LexWordAroundPos(CharPos(content, 'e'), content) == "_my_t5ype7");
  REQUIRE(LexWordAroundPos(CharPos(content, '7'), content) == "_my_t5ype7");
}

TEST_CASE("dot, dash, colon are skipped") {
  std::string content = "1. 2- 3:";
  REQUIRE(LexWordAroundPos(CharPos(content, '1'), content) == "1");
  REQUIRE(LexWordAroundPos(CharPos(content, '2'), content) == "2");
  REQUIRE(LexWordAroundPos(CharPos(content, '3'), content) == "3");
}

TEST_SUITE_END();



TEST_SUITE("FindIncludeLine");

TEST_CASE("in document") {
  std::vector<std::string> lines = {
    "#include <bbb>",   // 0
    "#include <ddd>"    // 1
  };

  REQUIRE(FindIncludeLine(lines, "#include <bbb>") == nullopt);
}

TEST_CASE("insert before") {
  std::vector<std::string> lines = {
    "#include <bbb>",   // 0
    "#include <ddd>"    // 1
  };

  REQUIRE(FindIncludeLine(lines, "#include <aaa>") == 0);
}

TEST_CASE("insert middle") {
  std::vector<std::string> lines = {
    "#include <bbb>",   // 0
    "#include <ddd>"    // 1
  };

  REQUIRE(FindIncludeLine(lines, "#include <ccc>") == 1);
}

TEST_CASE("insert after") {
  std::vector<std::string> lines = {
    "#include <bbb>",   // 0
    "#include <ddd>",   // 1
    "",                 // 2
  };

  REQUIRE(FindIncludeLine(lines, "#include <eee>") == 2);
}

TEST_CASE("ignore header") {
  std::vector<std::string> lines = {
    "// FOOBAR",        // 0
    "// FOOBAR",        // 1
    "// FOOBAR",        // 2
    "// FOOBAR",        // 3
    "",                 // 4
    "#include <bbb>",   // 5
    "#include <ddd>",   // 6
    "",                 // 7
  };

  REQUIRE(FindIncludeLine(lines, "#include <a>") == 5);
  REQUIRE(FindIncludeLine(lines, "#include <c>") == 6);
  REQUIRE(FindIncludeLine(lines, "#include <e>") == 7);
}

TEST_SUITE_END();
