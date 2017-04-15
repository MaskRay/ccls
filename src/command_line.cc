// TODO: cleanup includes
#include "cache.h"
#include "code_completion.h"
#include "file_consumer.h"
#include "indexer.h"
#include "query.h"
#include "language_server_api.h"
#include "options.h"
#include "project.h"
#include "platform.h"
#include "test.h"
#include "timer.h"
#include "threaded_queue.h"
#include "typed_bidi_message_queue.h"
#include "working_files.h"

#include <clang-c/Index.h>
#include <doctest/doctest.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <thread>
#include <vector>

// TODO: provide a feature like 'https://github.com/goldsborough/clang-expand',
// ie, a fully linear view of a function with inline function calls expanded.
// We can probably use vscode decorators to achieve it.

// TODO: we are not marking calls when an implicit ctors gets run. See
// GetDefinitionExtentOfSymbol as a good example.

namespace {

const char* kIpcLanguageClientName = "language_client";

const int kNumIndexers = 8 - 1;
const int kQueueSizeBytes = 1024 * 8;
const int kMaxWorkspaceSearchResults = 1000;

void PushBack(NonElidedVector<lsLocation>* result, optional<lsLocation> location) {
  if (location)
    result->push_back(*location);
}

QueryableFile* FindFile(QueryableDatabase* db, const std::string& filename, QueryFileId* file_id) {
  auto it = db->usr_to_symbol.find(filename);
  if (it != db->usr_to_symbol.end()) {
    *file_id = QueryFileId(it->second.idx);
    return &db->files[it->second.idx];
  }

  std::cerr << "Unable to find file " << filename << std::endl;
  *file_id = QueryFileId(-1);
  return nullptr;
}

QueryableFile* FindFile(QueryableDatabase* db, const std::string& filename) {
  // TODO: consider calling NormalizePath here. It might add too much latency though.
  auto it = db->usr_to_symbol.find(filename);
  if (it != db->usr_to_symbol.end())
    return &db->files[it->second.idx];

  std::cerr << "Unable to find file " << filename << std::endl;
  return nullptr;
}

QueryableFile* GetQueryable(QueryableDatabase* db, const QueryFileId& id) {
  return &db->files[id.id];
}
QueryableTypeDef* GetQueryable(QueryableDatabase* db, const QueryTypeId& id) {
  return &db->types[id.id];
}
QueryableFuncDef* GetQueryable(QueryableDatabase* db, const QueryFuncId& id) {
  return &db->funcs[id.id];
}
QueryableVarDef* GetQueryable(QueryableDatabase* db, const QueryVarId& id) {
  return &db->vars[id.id];
}

#if false
optional<QueryableLocation> GetDeclarationOfSymbol(QueryableDatabase* db, const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type:
      return db->types[symbol.idx].def.definition_spelling;
    case SymbolKind::Func:
      return db->funcs[symbol.idx].;
    case SymbolKind::Var:
      return db->vars[symbol.idx].uses;
    case SymbolKind::File:
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return {};
}
#endif



optional<QueryableLocation> GetDefinitionSpellingOfSymbol(QueryableDatabase* db, const QueryTypeId& id) {
  return GetQueryable(db, id)->def.definition_spelling;
}
optional<QueryableLocation> GetDefinitionSpellingOfSymbol(QueryableDatabase* db, const QueryFuncId& id) {
  return GetQueryable(db, id)->def.definition_spelling;
}
optional<QueryableLocation> GetDefinitionSpellingOfSymbol(QueryableDatabase* db, const QueryVarId& id) {
  return GetQueryable(db, id)->def.definition_spelling;
}
optional<QueryableLocation> GetDefinitionSpellingOfSymbol(QueryableDatabase* db, const SymbolIdx& symbol) {
  switch (symbol.kind) {
  case SymbolKind::Type:
    return db->types[symbol.idx].def.definition_spelling;
  case SymbolKind::Func:
    return db->funcs[symbol.idx].def.definition_spelling;
  case SymbolKind::Var:
    return db->vars[symbol.idx].def.definition_spelling;
  case SymbolKind::File:
  case SymbolKind::Invalid: {
    assert(false && "unexpected");
    break;
  }
  }
  return nullopt;
}


std::string GetHoverForSymbol(QueryableDatabase* db, const SymbolIdx& symbol) {
  switch (symbol.kind) {
  case SymbolKind::Type:
    return db->types[symbol.idx].def.qualified_name;
  case SymbolKind::Func:
    return db->funcs[symbol.idx].def.hover;
  case SymbolKind::Var:
    return db->vars[symbol.idx].def.hover;
  case SymbolKind::File:
  case SymbolKind::Invalid: {
    assert(false && "unexpected");
    break;
  }
  }
  return "";
}

std::vector<QueryableLocation> ToQueryableLocation(QueryableDatabase* db, const std::vector<QueryFuncRef>& refs) {
  std::vector<QueryableLocation> locs;
  locs.reserve(refs.size());
  for (const QueryFuncRef& ref : refs)
    locs.push_back(ref.loc);
  return locs;
}
std::vector<QueryableLocation> ToQueryableLocation(QueryableDatabase* db, const std::vector<QueryTypeId>& ids) {
  std::vector<QueryableLocation> locs;
  locs.reserve(ids.size());
  for (const QueryTypeId& id : ids) {
    optional<QueryableLocation> loc = GetDefinitionSpellingOfSymbol(db, id);
    if (loc)
      locs.push_back(loc.value());
  }
  return locs;
}
std::vector<QueryableLocation> ToQueryableLocation(QueryableDatabase* db, const std::vector<QueryFuncId>& ids) {
  std::vector<QueryableLocation> locs;
  locs.reserve(ids.size());
  for (const QueryFuncId& id : ids) {
    optional<QueryableLocation> loc = GetDefinitionSpellingOfSymbol(db, id);
    if (loc)
      locs.push_back(loc.value());
  }
  return locs;
}
std::vector<QueryableLocation> ToQueryableLocation(QueryableDatabase* db, const std::vector<QueryVarId>& ids) {
  std::vector<QueryableLocation> locs;
  locs.reserve(ids.size());
  for (const QueryVarId& id : ids) {
    optional<QueryableLocation> loc = GetDefinitionSpellingOfSymbol(db, id);
    if (loc)
      locs.push_back(loc.value());
  }
  return locs;
}



std::vector<QueryableLocation> GetUsesOfSymbol(QueryableDatabase* db, const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type:
      return db->types[symbol.idx].uses;
    case SymbolKind::Func: {
      // TODO: the vector allocation could be avoided.
      const QueryableFuncDef& func = db->funcs[symbol.idx];
      std::vector<QueryableLocation> result = ToQueryableLocation(db, func.callers);
      AddRange(&result, func.declarations);
      if (func.def.definition_spelling)
        result.push_back(*func.def.definition_spelling);
      return result;
    }
    case SymbolKind::Var:
      return db->vars[symbol.idx].uses;
    case SymbolKind::File:
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return {};
}

optional<QueryableLocation> GetDefinitionExtentOfSymbol(QueryableDatabase* db, const QueryTypeId& id) {
  return GetQueryable(db, id)->def.definition_extent;
}
optional<QueryableLocation> GetDefinitionExtentOfSymbol(QueryableDatabase* db, const QueryFuncId& id) {
  return GetQueryable(db, id)->def.definition_extent;
}
optional<QueryableLocation> GetDefinitionExtentOfSymbol(QueryableDatabase* db, const QueryVarId& id) {
  return GetQueryable(db, id)->def.definition_extent;
}
optional<QueryableLocation> GetDefinitionExtentOfSymbol(QueryableDatabase* db, const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::File:
      // TODO: If line 1 is deleted the file won't show up in, ie, workspace symbol search results.
      return QueryableLocation(QueryFileId(symbol.idx), Range(false /*is_interesting*/, Position(1, 1), Position(1, 1)));
    case SymbolKind::Type:
      return db->types[symbol.idx].def.definition_extent;
    case SymbolKind::Func:
      return db->funcs[symbol.idx].def.definition_extent;
    case SymbolKind::Var:
      return db->vars[symbol.idx].def.definition_extent;
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return nullopt;
}

std::vector<QueryableLocation> GetDeclarationsOfSymbolForGotoDefinition(QueryableDatabase* db, const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      // Returning the definition spelling of a type is a hack (and is why the
      // function has the postfix `ForGotoDefintion`, but it lets the user
      // jump to the start of a type if clicking goto-definition on the same
      // type from within the type definition.
      optional<QueryableLocation> declaration = db->types[symbol.idx].def.definition_spelling;
      if (declaration)
        return { *declaration };
      break;
    }
    case SymbolKind::Func:
      return db->funcs[symbol.idx].declarations;
    case SymbolKind::Var: {
      optional<QueryableLocation> declaration = db->vars[symbol.idx].def.declaration;
      if (declaration)
        return { *declaration };
      break;
    }
  }

  return {};
}

optional<QueryableLocation> GetBaseDefinitionOrDeclarationSpelling(QueryableDatabase* db, QueryableFuncDef& func) {
  if (!func.def.base)
    return nullopt;
  QueryableFuncDef& base = db->funcs[func.def.base->id];
  auto def = base.def.definition_spelling;
  if (!def && !base.declarations.empty())
    def = base.declarations[0];
  return def;
}

std::vector<QueryFuncRef> GetCallersForAllBaseFunctions(QueryableDatabase* db, QueryableFuncDef& root) {
  std::vector<QueryFuncRef> callers;

  optional<QueryFuncId> func_id = root.def.base;
  while (func_id) {
    QueryableFuncDef& def = db->funcs[func_id->id];
    AddRange(&callers, def.callers);
    func_id = def.def.base;
  }

  return callers;
}

std::vector<QueryFuncRef> GetCallersForAllDerivedFunctions(QueryableDatabase* db, QueryableFuncDef& root) {
  std::vector<QueryFuncRef> callers;

  std::queue<QueryFuncId> queue;
  PushRange(&queue, root.derived);

  while (!queue.empty()) {
    QueryableFuncDef& def = db->funcs[queue.front().id];
    queue.pop();
    PushRange(&queue, def.derived);

    AddRange(&callers, def.callers);
  }

  return callers;
}

optional<lsRange> GetLsRange(WorkingFile* working_file, const Range& location) {
  if (!working_file) {
    return lsRange(
      lsPosition(location.start.line - 1, location.start.column - 1),
      lsPosition(location.end.line - 1, location.end.column - 1));
  }

  // TODO: Should we also consider location.end.line?
  if (working_file->IsDeletedDiskLine(location.start.line))
    return nullopt;

  return lsRange(
    lsPosition(working_file->GetBufferLineFromDiskLine(location.start.line) - 1, location.start.column - 1),
    lsPosition(working_file->GetBufferLineFromDiskLine(location.end.line) - 1, location.end.column - 1));
}

lsDocumentUri GetLsDocumentUri(QueryableDatabase* db, QueryFileId file_id, std::string* path) {
  *path = db->files[file_id.id].def.usr;
  return lsDocumentUri::FromPath(*path);
}

lsDocumentUri GetLsDocumentUri(QueryableDatabase* db, QueryFileId file_id) {
  std::string path = db->files[file_id.id].def.usr;
  return lsDocumentUri::FromPath(path);
}

optional<lsLocation> GetLsLocation(QueryableDatabase* db, WorkingFiles* working_files, const QueryableLocation& location) {
  std::string path;
  lsDocumentUri uri = GetLsDocumentUri(db, location.path, &path);
  optional<lsRange> range = GetLsRange(working_files->GetFileByFilename(path), location.range);
  if (!range)
    return nullopt;
  return lsLocation(uri, *range);
}

// Returns a symbol. The symbol will have *NOT* have a location assigned.
lsSymbolInformation GetSymbolInfo(QueryableDatabase* db, WorkingFiles* working_files, SymbolIdx symbol) {
  lsSymbolInformation info;

  switch (symbol.kind) {
    case SymbolKind::File: {
      QueryableFile* def = symbol.ResolveFile(db);
      info.name = def->def.usr;
      info.kind = lsSymbolKind::File;
      break;
    }
    case SymbolKind::Type: {
      QueryableTypeDef* def = symbol.ResolveType(db);
      info.name = def->def.qualified_name;
      info.kind = lsSymbolKind::Class;
      break;
    }
    case SymbolKind::Func: {
      QueryableFuncDef* def = symbol.ResolveFunc(db);
      
      info.name = def->def.qualified_name;
      if (def->def.declaring_type.has_value()) {
        info.kind = lsSymbolKind::Method;
        info.containerName = db->types[def->def.declaring_type->id].def.qualified_name;
      }
      else {
        info.kind = lsSymbolKind::Function;
      }
      break;
    }
    case SymbolKind::Var: {
      QueryableVarDef* def = symbol.ResolveVar(db);
      info.name += def->def.qualified_name;
      info.kind = lsSymbolKind::Variable;
      break;
    }
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  };

  return info;
}

struct CommonCodeLensParams {
  std::vector<TCodeLens>* result;
  QueryableDatabase* db;
  WorkingFiles* working_files;
  WorkingFile* working_file;
};

void AddCodeLens(
  CommonCodeLensParams* common,
  QueryableLocation loc,
  const std::vector<QueryableLocation>& uses,
  const char* singular,
  const char* plural,
  bool exclude_loc = false,
  bool only_interesting = false) {
  TCodeLens code_lens;
  optional<lsRange> range = GetLsRange(common->working_file, loc.range);
  if (!range)
    return;
  code_lens.range = *range;
  code_lens.command = lsCommand<lsCodeLensCommandArguments>();
  code_lens.command->command = "superindex.showReferences";
  code_lens.command->arguments.uri = GetLsDocumentUri(common->db, loc.path);
  code_lens.command->arguments.position = code_lens.range.start;

  // Add unique uses.
  std::unordered_set<lsLocation> unique_uses;
  for (const QueryableLocation& use : uses) {
    if (exclude_loc && use == loc)
      continue;
    if (only_interesting && !use.range.interesting)
      continue;
    optional<lsLocation> location = GetLsLocation(common->db, common->working_files, use);
    if (!location)
      continue;
    unique_uses.insert(*location);
  }
  code_lens.command->arguments.locations.assign(unique_uses.begin(),
    unique_uses.end());

  // User visible label
  size_t num_usages = unique_uses.size();
  code_lens.command->title = std::to_string(num_usages) + " ";
  if (num_usages == 1)
    code_lens.command->title += singular;
  else
    code_lens.command->title += plural;

  if (exclude_loc || unique_uses.size() > 0)
    common->result->push_back(code_lens);
}

lsWorkspaceEdit BuildWorkspaceEdit(QueryableDatabase* db, WorkingFiles* working_files, const std::vector<QueryableLocation>& locations, const std::string& new_text) {
  std::unordered_map<QueryFileId, lsTextDocumentEdit> path_to_edit;

  for (auto& location : locations) {
    optional<lsLocation> ls_location = GetLsLocation(db, working_files, location);
    if (!ls_location)
      continue;
    
    if (path_to_edit.find(location.path) == path_to_edit.end()) {
      path_to_edit[location.path] = lsTextDocumentEdit();

      const std::string& path = db->files[location.path.id].def.usr;
      path_to_edit[location.path].textDocument.uri = lsDocumentUri::FromPath(path);
      
      WorkingFile* working_file = working_files->GetFileByFilename(path);
      if (working_file)
        path_to_edit[location.path].textDocument.version = working_file->version;
    }

    lsTextEdit edit;
    edit.range = ls_location->range;
    edit.newText = new_text;
    path_to_edit[location.path].edits.push_back(edit);
  }


  lsWorkspaceEdit edit;
  for (const auto& changes : path_to_edit)
    edit.documentChanges.push_back(changes.second);
  return edit;
}

}  // namespace























struct Index_DoIndex {
  enum class Type {
    Import,
    Update
  };

  std::string path;
  std::vector<std::string> args;
  Type type;

  Index_DoIndex(Type type) : type(type) {}
};

struct Index_DoIdMap {
  std::unique_ptr<IndexedFile> previous;
  std::unique_ptr<IndexedFile> current;

  explicit Index_DoIdMap(std::unique_ptr<IndexedFile> previous,
                         std::unique_ptr<IndexedFile> current)
    : previous(std::move(previous)),
      current(std::move(current)) {}
};

struct Index_OnIdMapped {
  std::unique_ptr<IndexedFile> previous_index;
  std::unique_ptr<IndexedFile> current_index;
  std::unique_ptr<IdMap> previous_id_map;
  std::unique_ptr<IdMap> current_id_map;
};

struct Index_OnIndexed {
  IndexUpdate update;
  explicit Index_OnIndexed(IndexUpdate& update) : update(update) {}
};

// TODO: Rename TypedBidiMessageQueue to IpcTransport?
using IpcMessageQueue = TypedBidiMessageQueue<IpcId, BaseIpcMessage>;
using Index_DoIndexQueue = ThreadedQueue<Index_DoIndex>;
using Index_DoIdMapQueue = ThreadedQueue<Index_DoIdMap>;
using Index_OnIdMappedQueue = ThreadedQueue<Index_OnIdMapped>;
using Index_OnIndexedQueue = ThreadedQueue<Index_OnIndexed>;

template<typename TMessage>
void SendMessage(IpcMessageQueue& t, MessageQueue* destination, TMessage& message) {
  t.SendMessage(destination, TMessage::kIpcId, message);
}

template<typename T>
void SendOutMessageToClient(IpcMessageQueue* queue, T& response) {
  std::ostringstream sstream;
  response.Write(sstream);

  Ipc_Cout out;
  out.content = sstream.str();
  queue->SendMessage(&queue->for_client, Ipc_Cout::kIpcId, out);
}

template<typename T>
void RegisterId(IpcMessageQueue* t) {
  t->RegisterId(T::kIpcId,
    [](Writer& visitor, BaseIpcMessage& message) {
    T& m = static_cast<T&>(message);
    Reflect(visitor, m);
  }, [](Reader& visitor) {
    auto m = MakeUnique<T>();
    Reflect(visitor, *m);
    return m;
  });
}

std::unique_ptr<IpcMessageQueue> BuildIpcMessageQueue(const std::string& name, size_t buffer_size) {
  auto ipc = MakeUnique<IpcMessageQueue>(name, buffer_size);
  RegisterId<Ipc_CancelRequest>(ipc.get());
  RegisterId<Ipc_InitializeRequest>(ipc.get());
  RegisterId<Ipc_InitializedNotification>(ipc.get());
  RegisterId<Ipc_TextDocumentDidOpen>(ipc.get());
  RegisterId<Ipc_TextDocumentDidChange>(ipc.get());
  RegisterId<Ipc_TextDocumentDidClose>(ipc.get());
  RegisterId<Ipc_TextDocumentDidSave>(ipc.get());
  RegisterId<Ipc_TextDocumentRename>(ipc.get());
  RegisterId<Ipc_TextDocumentComplete>(ipc.get());
  RegisterId<Ipc_TextDocumentDefinition>(ipc.get());
  RegisterId<Ipc_TextDocumentDocumentHighlight>(ipc.get());
  RegisterId<Ipc_TextDocumentHover>(ipc.get());
  RegisterId<Ipc_TextDocumentReferences>(ipc.get());
  RegisterId<Ipc_TextDocumentDocumentSymbol>(ipc.get());
  RegisterId<Ipc_TextDocumentCodeLens>(ipc.get());
  RegisterId<Ipc_CodeLensResolve>(ipc.get());
  RegisterId<Ipc_WorkspaceSymbol>(ipc.get());
  RegisterId<Ipc_Quit>(ipc.get());
  RegisterId<Ipc_IsAlive>(ipc.get());
  RegisterId<Ipc_OpenProject>(ipc.get());
  RegisterId<Ipc_Cout>(ipc.get());
  return ipc;
}

void RegisterMessageTypes() {
  MessageRegistry::instance()->Register<Ipc_CancelRequest>();
  MessageRegistry::instance()->Register<Ipc_InitializeRequest>();
  MessageRegistry::instance()->Register<Ipc_InitializedNotification>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDidOpen>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDidChange>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDidClose>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDidSave>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentRename>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentComplete>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDefinition>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDocumentHighlight>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentHover>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentReferences>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDocumentSymbol>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentCodeLens>();
  MessageRegistry::instance()->Register<Ipc_CodeLensResolve>();
  MessageRegistry::instance()->Register<Ipc_WorkspaceSymbol>();
}

bool IndexMain_DoIndex(FileConsumer::SharedState* file_consumer_shared,
                       Index_DoIndexQueue* queue_do_index,
                       Index_DoIdMapQueue* queue_do_id_map) {
  optional<Index_DoIndex> index_request = queue_do_index->TryDequeue();
  if (!index_request)
    return false;

  Timer time;

  // TODO TODO TODO TODO TODO
  // TODO TODO TODO TODO TODO
  // TODO TODO TODO TODO TODO
  // TODO TODO TODO TODO TODO
  // We're not loading cached header files on restore. We should store the
  // list of headers associated with a cc file in the cache and then load
  // them here.
  // TODO TODO TODO TODO TODO
  // TODO TODO TODO TODO TODO
  // TODO TODO TODO TODO TODO
  // TODO TODO TODO TODO TODO



  // If the index update is an import, then we will load the previous index
  // into memory if we have a previous index. After that, we dispatch an
  // update request to get the latest version.
  if (index_request->type == Index_DoIndex::Type::Import) {
    index_request->type = Index_DoIndex::Type::Update;
    std::unique_ptr<IndexedFile> old_index = LoadCachedFile(index_request->path);
    time.ResetAndPrint("Reading cached index from disk " + index_request->path);

    // If import fails just do a standard update.
    if (old_index) {
      for (auto& dependency_path : old_index->dependencies) {
        std::cerr << "- Dispatching dependency import " << dependency_path << std::endl;
        Index_DoIndex dep_index_request(Index_DoIndex::Type::Import);
        dep_index_request.path = dependency_path;
        dep_index_request.args = index_request->args;
        queue_do_index->Enqueue(std::move(dep_index_request));
      }


      Index_DoIdMap response(nullptr, std::move(old_index));
      queue_do_id_map->Enqueue(std::move(response));

      queue_do_index->Enqueue(std::move(*index_request));
      return true;
    }
  }

  // Parse request and send a response.
  std::vector<std::unique_ptr<IndexedFile>> indexes = Parse(file_consumer_shared, index_request->path, index_request->args);
  time.ResetAndPrint("Parsing/indexing " + index_request->path);

  for (auto& current_index : indexes) {
    std::cerr << "Got index for " << current_index->path << std::endl;

    std::unique_ptr<IndexedFile> old_index = LoadCachedFile(current_index->path);
    time.ResetAndPrint("Loading cached index");

    // TODO: Cache to disk on a separate thread. Maybe we do the cache after we
    // have imported the index (so the import pipeline has five stages instead
    // of the current 4).

    // Cache file so we can diff it later.
    WriteToCache(current_index->path, *current_index);
    time.ResetAndPrint("Cache index update to disk");

    // Send response to create id map.
    Index_DoIdMap response(std::move(old_index), std::move(current_index));
    queue_do_id_map->Enqueue(std::move(response));
  }

  return true;
}

bool IndexMain_DoCreateIndexUpdate(
    Index_OnIdMappedQueue* queue_on_id_mapped,
    Index_OnIndexedQueue* queue_on_indexed) {
  optional<Index_OnIdMapped> response = queue_on_id_mapped->TryDequeue();
  if (!response)
    return false;

  Timer time;
  IndexUpdate update = IndexUpdate::CreateDelta(response->previous_id_map.get(), response->current_id_map.get(),
    response->previous_index.get(), response->current_index.get());
  time.ResetAndPrint("Creating delta IndexUpdate");
  Index_OnIndexed reply(update);
  queue_on_indexed->Enqueue(std::move(reply));
  time.ResetAndPrint("Sending update to server");

  return true;
}

void IndexJoinIndexUpdates(Index_OnIndexedQueue* queue_on_indexed) {
  optional<Index_OnIndexed> root = queue_on_indexed->TryDequeue();
  if (!root)
    return;

  while (true) {
    optional<Index_OnIndexed> to_join = queue_on_indexed->TryDequeue();
    if (!to_join) {
      queue_on_indexed->Enqueue(std::move(*root));
      return;
    }

    Timer time;
    root->update.Merge(to_join->update);
    time.ResetAndPrint("Indexer joining two querydb updates");
  }
}

void IndexMain(
  FileConsumer::SharedState* file_consumer_shared,
  Index_DoIndexQueue* queue_do_index,
  Index_DoIdMapQueue* queue_do_id_map,
  Index_OnIdMappedQueue* queue_on_id_mapped,
  Index_OnIndexedQueue* queue_on_indexed) {

  while (true) {
    // TODO: process all off IndexMain_DoIndex before calling IndexMain_DoCreateIndexUpdate for
    //       better icache behavior. We need to have some threads spinning on both though
    //       otherwise memory usage will get bad.

    int count = 0;

    if (!IndexMain_DoIndex(file_consumer_shared, queue_do_index, queue_do_id_map) &&
      !IndexMain_DoCreateIndexUpdate(queue_on_id_mapped, queue_on_indexed)) {

      //if (count++ > 2) {
      //  count = 0;
        IndexJoinIndexUpdates(queue_on_indexed);
      //}

      // TODO: use CV to wakeup?
      std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
  }
}










































void QueryDbMainLoop(
  QueryableDatabase* db,
  IpcMessageQueue* language_client,
  Index_DoIndexQueue* queue_do_index,
  Index_DoIdMapQueue* queue_do_id_map,
  Index_OnIdMappedQueue* queue_on_id_mapped,
  Index_OnIndexedQueue* queue_on_indexed,
  Project* project,
  WorkingFiles* working_files,
  CompletionManager* completion_manager) {

  std::vector<std::unique_ptr<BaseIpcMessage>> messages = language_client->GetMessages(&language_client->for_server);
  for (auto& message : messages) {
    //std::cerr << "[querydb] Processing message " << static_cast<int>(message->method_id) << std::endl;

    switch (message->method_id) {
    case IpcId::Quit: {
      std::cerr << "Got quit message (exiting)" << std::endl;
      exit(0);
      break;
    }

    case IpcId::IsAlive: {
      Ipc_IsAlive response;
      language_client->SendMessage(&language_client->for_client, response.method_id, response);
      break;
    }

    case IpcId::OpenProject: {
      Ipc_OpenProject* msg = static_cast<Ipc_OpenProject*>(message.get());
      std::string path = msg->project_path;

      project->Load(path);
      std::cerr << "Loaded compilation entries (" << project->entries.size() << " files)" << std::endl;
      //for (int i = 0; i < 10; ++i)
        //std::cerr << project->entries[i].filename << std::endl;
      for (int i = 0; i < project->entries.size(); ++i) {
        const CompilationEntry& entry = project->entries[i];
        std::string filepath = entry.filename;

        std::cerr << "[" << i << "/" << (project->entries.size() - 1)
          << "] Dispatching index request for file " << filepath
          << std::endl;

        Index_DoIndex request(Index_DoIndex::Type::Import);
        request.path = filepath;
        request.args = entry.args;
        queue_do_index->Enqueue(std::move(request));
      }
      break;
    }

    case IpcId::TextDocumentDidOpen: {
      auto msg = static_cast<Ipc_TextDocumentDidOpen*>(message.get());
      //std::cerr << "Opening " << msg->params.textDocument.uri.GetPath() << std::endl;
      working_files->OnOpen(msg->params);
      break;
    }
    case IpcId::TextDocumentDidChange: {
      auto msg = static_cast<Ipc_TextDocumentDidChange*>(message.get());
      working_files->OnChange(msg->params);
      //std::cerr << "Changing " << msg->params.textDocument.uri.GetPath() << std::endl;
      break;
    }
    case IpcId::TextDocumentDidClose: {
      auto msg = static_cast<Ipc_TextDocumentDidClose*>(message.get());
      std::cerr << "Closing " << msg->params.textDocument.uri.GetPath() << std::endl;
      working_files->OnClose(msg->params);
      break;
    }

    case IpcId::TextDocumentDidSave: {
      auto msg = static_cast<Ipc_TextDocumentDidSave*>(message.get());

      WorkingFile* working_file = working_files->GetFileByFilename(msg->params.textDocument.uri.GetPath());
      if (working_file)
        working_file->changes.clear();

      // Send an index update request.
      Index_DoIndex request(Index_DoIndex::Type::Update);
      optional<CompilationEntry> entry = project->FindCompilationEntryForFile(msg->params.textDocument.uri.GetPath());
      request.path = msg->params.textDocument.uri.GetPath();
      if (entry)
        request.args = entry->args;
      queue_do_index->Enqueue(std::move(request));
      break;
    }

    case IpcId::TextDocumentRename: {
      auto msg = static_cast<Ipc_TextDocumentRename*>(message.get());

      QueryFileId file_id;
      QueryableFile* file = FindFile(db, msg->params.textDocument.uri.GetPath(), &file_id);
      if (!file) {
        std::cerr << "Unable to find file " << msg->params.textDocument.uri.GetPath() << std::endl;
        break;
      }
      Out_TextDocumentRename response;
      response.id = msg->id;

      // TODO: consider refactoring into FindSymbolsAtLocation(file);
      int target_line = msg->params.position.line + 1;
      int target_column = msg->params.position.character + 1;
      for (const SymbolRef& ref : file->def.all_symbols) {
        if (ref.loc.range.start.line >= target_line && ref.loc.range.end.line <= target_line &&
          ref.loc.range.start.column <= target_column && ref.loc.range.end.column >= target_column) {

          // Found symbol. Return references to rename.
          std::vector<QueryableLocation> uses = GetUsesOfSymbol(db, ref.idx);
          response.result = BuildWorkspaceEdit(db, working_files, uses, msg->params.newName);
          break;
        }
      }

      response.Write(std::cerr);
      SendOutMessageToClient(language_client, response);
      break;
    }

    case IpcId::TextDocumentCompletion: {
      // TODO: better performance
      auto msg = static_cast<Ipc_TextDocumentComplete*>(message.get());
      Out_TextDocumentComplete response;
      response.id = msg->id;
      response.result.isIncomplete = false;
      response.result.items = completion_manager->CodeComplete(msg->params);

      Timer timer;
      response.Write(std::cout);
      timer.ResetAndPrint("Writing completion results");
      //SendOutMessageToClient(language_client, response);
      break;
    }

    case IpcId::TextDocumentDefinition: {
      auto msg = static_cast<Ipc_TextDocumentDefinition*>(message.get());

      QueryFileId file_id;
      QueryableFile* file = FindFile(db, msg->params.textDocument.uri.GetPath(), &file_id);
      if (!file) {
        std::cerr << "Unable to find file " << msg->params.textDocument.uri.GetPath() << std::endl;
        break;
      }

      Out_TextDocumentDefinition response;
      response.id = msg->id;

      int target_line = msg->params.position.line + 1;
      int target_column = msg->params.position.character + 1;

      for (const SymbolRef& ref : file->def.all_symbols) {
        if (ref.loc.range.Contains(target_line, target_column)) {
          // Found symbol. Return definition.

          // Special cases which are handled:
          //  - symbol has declaration but no definition (ie, pure virtual)
          //  - start at spelling but end at extent for better mouse tooltip
          //  - goto declaration while in definition of recursive type

          optional<QueryableLocation> def_loc = GetDefinitionSpellingOfSymbol(db, ref.idx);

          // We use spelling start and extent end because this causes vscode to
          // highlight the entire definition when previewing / hoving with the
          // mouse.
          optional<QueryableLocation> def_extent = GetDefinitionExtentOfSymbol(db, ref.idx);
          if (def_loc && def_extent)
            def_loc->range.end = def_extent->range.end;

          // If the cursor is currently at or in the definition we should goto
          // the declaration if possible. We also want to use declarations if
          // we're pointing to, ie, a pure virtual function which has no
          // definition.
          if (!def_loc || (def_loc->path == file_id &&
                           def_loc->range.Contains(target_line, target_column))) {
            // Goto declaration.

            std::vector<QueryableLocation> declarations = GetDeclarationsOfSymbolForGotoDefinition(db, ref.idx);
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
      }

      SendOutMessageToClient(language_client, response);
      break;
    }

    case IpcId::TextDocumentDocumentHighlight: {
      auto msg = static_cast<Ipc_TextDocumentDocumentHighlight*>(message.get());

      QueryFileId file_id;
      QueryableFile* file = FindFile(db, msg->params.textDocument.uri.GetPath(), &file_id);
      if (!file) {
        std::cerr << "Unable to find file " << msg->params.textDocument.uri.GetPath() << std::endl;
        break;
      }
      Out_TextDocumentDocumentHighlight response;
      response.id = msg->id;

      // TODO: consider refactoring into FindSymbolsAtLocation(file);
      int target_line = msg->params.position.line + 1;
      int target_column = msg->params.position.character + 1;
      for (const SymbolRef& ref : file->def.all_symbols) {
        if (ref.loc.range.start.line >= target_line && ref.loc.range.end.line <= target_line &&
          ref.loc.range.start.column <= target_column && ref.loc.range.end.column >= target_column) {

          // Found symbol. Return references to highlight.
          std::vector<QueryableLocation> uses = GetUsesOfSymbol(db, ref.idx);
          response.result.reserve(uses.size());
          for (const QueryableLocation& use : uses) {
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
      }

      SendOutMessageToClient(language_client, response);
      break;
    }

    case IpcId::TextDocumentHover: {
      auto msg = static_cast<Ipc_TextDocumentHover*>(message.get());

      QueryableFile* file = FindFile(db, msg->params.textDocument.uri.GetPath());
      if (!file) {
        std::cerr << "Unable to find file " << msg->params.textDocument.uri.GetPath() << std::endl;
        break;
      }
      Out_TextDocumentHover response;
      response.id = msg->id;

      // TODO: consider refactoring into FindSymbolsAtLocation(file);
      int target_line = msg->params.position.line + 1;
      int target_column = msg->params.position.character + 1;
      for (const SymbolRef& ref : file->def.all_symbols) {
        if (ref.loc.range.start.line >= target_line && ref.loc.range.end.line <= target_line &&
          ref.loc.range.start.column <= target_column && ref.loc.range.end.column >= target_column) {

          // Found symbol. Return hover.
          optional<lsRange> ls_range = GetLsRange(working_files->GetFileByFilename(file->def.usr), ref.loc.range);
          if (!ls_range)
            continue;

          response.result.contents = GetHoverForSymbol(db, ref.idx);
          response.result.range = *ls_range;
          break;
        }
      }

      SendOutMessageToClient(language_client, response);
      break;
    }

    case IpcId::TextDocumentReferences: {
      auto msg = static_cast<Ipc_TextDocumentReferences*>(message.get());

      QueryableFile* file = FindFile(db, msg->params.textDocument.uri.GetPath());
      if (!file) {
        std::cerr << "Unable to find file " << msg->params.textDocument.uri.GetPath() << std::endl;
        break;
      }

      Out_TextDocumentReferences response;
      response.id = msg->id;

      // TODO: consider refactoring into FindSymbolsAtLocation(file);
      int target_line = msg->params.position.line + 1;
      int target_column = msg->params.position.character + 1;
      for (const SymbolRef& ref : file->def.all_symbols) {
        if (ref.loc.range.start.line >= target_line && ref.loc.range.end.line <= target_line &&
            ref.loc.range.start.column <= target_column && ref.loc.range.end.column >= target_column) {

          optional<QueryableLocation> excluded_declaration;
          if (!msg->params.context.includeDeclaration) {
            std::cerr << "Excluding declaration in references" << std::endl;
            excluded_declaration = GetDefinitionSpellingOfSymbol(db, ref.idx);
          }

          // Found symbol. Return references.
          std::vector<QueryableLocation> uses = GetUsesOfSymbol(db, ref.idx);
          response.result.reserve(uses.size());
          for (const QueryableLocation& use : uses) {
            if (excluded_declaration.has_value() && use == *excluded_declaration)
              continue;

            optional<lsLocation> ls_location = GetLsLocation(db, working_files, use);
            if (ls_location)
              response.result.push_back(*ls_location);
          }
          break;
        }
      }

      SendOutMessageToClient(language_client, response);
      break;
    }

    case IpcId::TextDocumentDocumentSymbol: {
      auto msg = static_cast<Ipc_TextDocumentDocumentSymbol*>(message.get());

      Out_TextDocumentDocumentSymbol response;
      response.id = msg->id;

      QueryableFile* file = FindFile(db, msg->params.textDocument.uri.GetPath());
      if (!file) {
        std::cerr << "Unable to find file " << msg->params.textDocument.uri.GetPath() << std::endl;
        break;
      }

      std::cerr << "File outline size is " << file->def.outline.size() << std::endl;
      for (SymbolRef ref : file->def.outline) {
        lsSymbolInformation info = GetSymbolInfo(db, working_files, ref.idx);
        optional<lsLocation> location = GetLsLocation(db, working_files, ref.loc);
        if (!location)
          continue;
        info.location = *location;
        response.result.push_back(info);
      }

      SendOutMessageToClient(language_client, response);
      break;
    }

    case IpcId::TextDocumentCodeLens: {
      auto msg = static_cast<Ipc_TextDocumentCodeLens*>(message.get());

      Out_TextDocumentCodeLens response;
      response.id = msg->id;

      lsDocumentUri file_as_uri = msg->params.textDocument.uri;

      QueryableFile* file = FindFile(db, file_as_uri.GetPath());
      if (!file) {
        std::cerr << "Unable to find file " << msg->params.textDocument.uri.GetPath() << std::endl;
        break;
      }
      CommonCodeLensParams common;
      common.result = &response.result;
      common.db = db;
      common.working_files = working_files;
      common.working_file = working_files->GetFileByFilename(file->def.usr);

      for (SymbolRef ref : file->def.outline) {
        // NOTE: We OffsetColumn so that the code lens always show up in a
        // predictable order. Otherwise, the client may randomize it.

        SymbolIdx symbol = ref.idx;
        switch (symbol.kind) {
        case SymbolKind::Type: {
          QueryableTypeDef& def = db->types[symbol.idx];
          AddCodeLens(&common, ref.loc.OffsetStartColumn(0), def.uses, "ref", "refs");
          AddCodeLens(&common, ref.loc.OffsetStartColumn(1), def.uses, "iref", "irefs", false /*exclude_loc*/, true /*only_interesting*/);
          AddCodeLens(&common, ref.loc.OffsetStartColumn(2), ToQueryableLocation(db, def.derived), "derived", "derived");
          AddCodeLens(&common, ref.loc.OffsetStartColumn(3), ToQueryableLocation(db, def.instantiations), "instantiation", "instantiations");
          break;
        }
        case SymbolKind::Func: {
          QueryableFuncDef& func = db->funcs[symbol.idx];


          int offset = 0;

          std::vector<QueryFuncRef> base_callers = GetCallersForAllBaseFunctions(db, func);
          std::vector<QueryFuncRef> derived_callers = GetCallersForAllDerivedFunctions(db, func);
          if (base_callers.empty() && derived_callers.empty()) {
            // set exclude_loc to true to force the code lens to show up
            AddCodeLens(&common, ref.loc.OffsetStartColumn(offset++), ToQueryableLocation(db, func.callers), "call", "calls", true /*exclude_loc*/);
          }
          else {
            AddCodeLens(&common, ref.loc.OffsetStartColumn(offset++), ToQueryableLocation(db, func.callers), "direct call", "direct calls");
            if (!base_callers.empty())
              AddCodeLens(&common, ref.loc.OffsetStartColumn(offset++), ToQueryableLocation(db, base_callers), "base call", "base calls");
            if (!derived_callers.empty())
              AddCodeLens(&common, ref.loc.OffsetStartColumn(offset++), ToQueryableLocation(db, derived_callers), "derived call", "derived calls");
          }

          AddCodeLens(&common, ref.loc.OffsetStartColumn(offset++), ToQueryableLocation(db, func.derived), "derived", "derived");

          // "Base"
          optional<QueryableLocation> base_loc = GetBaseDefinitionOrDeclarationSpelling(db, func);
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
                code_lens.command->command = "superindex.goto";
                code_lens.command->arguments.uri = ls_base->uri;
                code_lens.command->arguments.position = ls_base->range.start;
                response.result.push_back(code_lens);
              }
            }
          }

          break;
        }
        case SymbolKind::Var: {
          QueryableVarDef& def = db->vars[symbol.idx];
          AddCodeLens(&common, ref.loc.OffsetStartColumn(0), def.uses, "ref", "refs", true /*exclude_loc*/, false /*only_interesting*/);
          break;
        }
        case SymbolKind::File:
        case SymbolKind::Invalid: {
          assert(false && "unexpected");
          break;
        }
        };
      }

      SendOutMessageToClient(language_client, response);
      break;
    }

    case IpcId::WorkspaceSymbol: {
      auto msg = static_cast<Ipc_WorkspaceSymbol*>(message.get());

      Out_WorkspaceSymbol response;
      response.id = msg->id;


      std::cerr << "- Considering " << db->qualified_names.size()
        << " candidates for query " << msg->params.query << std::endl;

      std::string query = msg->params.query;
      for (int i = 0; i < db->qualified_names.size(); ++i) {
        if (response.result.size() > kMaxWorkspaceSearchResults) {
          std::cerr << "Query exceeded maximum number of responses (" << kMaxWorkspaceSearchResults << "), output may not contain all results" << std::endl;
          break;
        }

        if (db->qualified_names[i].find(query) != std::string::npos) {
          lsSymbolInformation info = GetSymbolInfo(db, working_files, db->symbols[i]);
          optional<QueryableLocation> location = GetDefinitionExtentOfSymbol(db, db->symbols[i]);
          if (!location) {
            auto decls = GetDeclarationsOfSymbolForGotoDefinition(db, db->symbols[i]);
            if (decls.empty())
              continue;
            location = decls[0];
          }

          optional<lsLocation> ls_location = GetLsLocation(db, working_files, *location);
          if (!ls_location)
            continue;
          info.location = *ls_location;
          response.result.push_back(info);
        }
      }

      std::cerr << "- Found " << response.result.size() << " results" << std::endl;
      SendOutMessageToClient(language_client, response);
      break;
    }

    default: {
      std::cerr << "Unhandled IPC message with kind "
        << static_cast<int>(message->method_id) << std::endl;
      exit(1);
    }
    }
  }

  // TODO: consider rate-limiting and checking for IPC messages so we don't block
  // requests / we can serve partial requests.


  while (true) {
    optional<Index_DoIdMap> request = queue_do_id_map->TryDequeue();
    if (!request)
      break;


    Index_OnIdMapped response;
    Timer time;
    if (request->previous) {
      response.previous_id_map = MakeUnique<IdMap>(db, request->previous->id_cache);
      response.previous_index = std::move(request->previous);
    }

    assert(request->current);
    response.current_id_map = MakeUnique<IdMap>(db, request->current->id_cache);
    time.ResetAndPrint("Create IdMap " + request->current->path);
    response.current_index = std::move(request->current);

    queue_on_id_mapped->Enqueue(std::move(response));
  }

  while (true) {
    optional<Index_OnIndexed> response = queue_on_indexed->TryDequeue();
    if (!response)
      break;

    Timer time;
    db->ApplyIndexUpdate(&response->update);
    time.ResetAndPrint("Applying index update");
  }
}

void QueryDbMain() {
  //std::cerr << "Running QueryDb" << std::endl;

  // Create queues.
  std::unique_ptr<IpcMessageQueue> ipc = BuildIpcMessageQueue(kIpcLanguageClientName, kQueueSizeBytes);
  Index_DoIndexQueue queue_do_index;
  Index_DoIdMapQueue queue_do_id_map;
  Index_OnIdMappedQueue queue_on_id_mapped;
  Index_OnIndexedQueue queue_on_indexed;

  Project project;
  WorkingFiles working_files;
  CompletionManager completion_manager(&project, &working_files);
  FileConsumer::SharedState file_consumer_shared;

  // Start indexer threads.
  for (int i = 0; i < kNumIndexers; ++i) {
    new std::thread([&]() {
      IndexMain(&file_consumer_shared, &queue_do_index, &queue_do_id_map, &queue_on_id_mapped, &queue_on_indexed);
    });
  }

  // Run query db main loop.
  QueryableDatabase db;
  while (true) {
    QueryDbMainLoop(&db, ipc.get(), &queue_do_index, &queue_do_id_map, &queue_on_id_mapped, &queue_on_indexed, &project, &working_files, &completion_manager);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

// TODO: global lock on stderr output.

// Separate thread whose only job is to read from stdin and
// dispatch read commands to the actual indexer program. This
// cannot be done on the main thread because reading from std::cin
// blocks.
//
// |ipc| is connected to a server.
void LanguageServerStdinLoop(IpcMessageQueue* ipc) {
  while (true) {
    std::unique_ptr<BaseIpcMessage> message = MessageRegistry::instance()->ReadMessageFromStdin();

    // Message parsing can fail if we don't recognize the method.
    if (!message)
      continue;

    //std::cerr << "[info]: Got message of type "
    //  << IpcIdToString(message->method_id) << std::endl;
    switch (message->method_id) {
      // TODO: For simplicitly lets just proxy the initialize request like
      // all other requests so that stdin loop thread becomes super simple.
    case IpcId::Initialize: {
      auto request = static_cast<Ipc_InitializeRequest*>(message.get());
      if (request->params.rootUri) {
        std::string project_path = request->params.rootUri->GetPath();
        std::cerr << "Initialize in directory " << project_path
          << " with uri " << request->params.rootUri->raw_uri
          << std::endl;
        Ipc_OpenProject open_project;
        open_project.project_path = project_path;
        ipc->SendMessage(&ipc->for_server, Ipc_OpenProject::kIpcId, open_project);
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
      response.result.capabilities.completionProvider->triggerCharacters = { ".", "::", "->" };

      response.result.capabilities.codeLensProvider = lsCodeLensOptions();
      response.result.capabilities.codeLensProvider->resolveProvider = false;

      response.result.capabilities.definitionProvider = true;
      response.result.capabilities.documentHighlightProvider = true;
      response.result.capabilities.hoverProvider = true;
      response.result.capabilities.referencesProvider = true;

      response.result.capabilities.documentSymbolProvider = true;
      response.result.capabilities.workspaceSymbolProvider = true;

      //response.Write(std::cerr);
      response.Write(std::cout);
      break;
    }

    case IpcId::Initialized: {
      // TODO: don't send output until we get this notification
      break;
    }

    case IpcId::CancelRequest: {
      // TODO: support cancellation
      break;
    }

    case IpcId::TextDocumentDidOpen:
    case IpcId::TextDocumentDidChange:
    case IpcId::TextDocumentDidClose:
    case IpcId::TextDocumentDidSave:
    case IpcId::TextDocumentRename:
    case IpcId::TextDocumentCompletion:
    case IpcId::TextDocumentDefinition:
    case IpcId::TextDocumentDocumentHighlight:
    case IpcId::TextDocumentHover:
    case IpcId::TextDocumentReferences:
    case IpcId::TextDocumentDocumentSymbol:
    case IpcId::TextDocumentCodeLens:
    case IpcId::WorkspaceSymbol: {
      //std::cerr << "Sending message " << (int)message->method_id << std::endl;
      ipc->SendMessage(&ipc->for_server, message->method_id, *message.get());
      break;
    }

    default: {
      std::cerr << "Unhandled IPC message with kind "
        << static_cast<int>(message->method_id) << std::endl;
      exit(1);
    }
    }
  }
}

void LanguageServerMainLoop(IpcMessageQueue* ipc) {
  std::vector<std::unique_ptr<BaseIpcMessage>> messages = ipc->GetMessages(&ipc->for_client);
  for (auto& message : messages) {
    switch (message->method_id) {
    case IpcId::Quit: {
      std::cerr << "Got quit message (exiting)" << std::endl;
      exit(0);
      break;
    }

    case IpcId::Cout: {
      auto msg = static_cast<Ipc_Cout*>(message.get());
      std::cout << msg->content;
      std::cout.flush();
      break;
    }

    default: {
      std::cerr << "Unhandled IPC message with kind "
        << static_cast<int>(message->method_id) << std::endl;
      exit(1);
    }
    }
  }
}

bool IsQueryDbProcessRunning(IpcMessageQueue* ipc) {
  // Emit an alive check. Sleep so the server has time to respond.
  Ipc_IsAlive check_alive;
  SendMessage(*ipc, &ipc->for_server, check_alive);

  // TODO: Tune this value or make it configurable.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Check if we got an IsAlive message back.
  std::vector<std::unique_ptr<BaseIpcMessage>> messages = ipc->GetMessages(&ipc->for_client);
  for (auto& message : messages) {
    if (IpcId::IsAlive == message->method_id)
      return true;
  }

  return false;
}

void LanguageServerMain(std::string process_name) {
  std::unique_ptr<IpcMessageQueue> ipc = BuildIpcMessageQueue(kIpcLanguageClientName, kQueueSizeBytes);

  // Discard any left-over messages from previous runs.
  ipc->GetMessages(&ipc->for_client);

  bool has_server = IsQueryDbProcessRunning(ipc.get());

  // No server is running. Start it in-process. If the user wants to run the
  // server out of process they have to start it themselves.
  if (!has_server) {
    new std::thread(&QueryDbMain);
  }

  // Run language client.
  new std::thread(&LanguageServerStdinLoop, ipc.get());
  while (true) {
    LanguageServerMainLoop(ipc.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

int main(int argc, char** argv) {
  clang_enableStackTraces();
  clang_toggleCrashRecovery(1);


  //bool loop = true;
  //while (loop)
  //  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  //std::this_thread::sleep_for(std::chrono::seconds(3));

  PlatformInit();
  RegisterMessageTypes();

  // if (argc == 1) {
  //  QueryDbMain();
  //  return 0;
  //}


  std::unordered_map<std::string, std::string> options =
    ParseOptions(argc, argv);

  if (argc == 1 || HasOption(options, "--test")) {
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    int res = context.run();
    if (context.shouldExit())
      return res;

    RunTests();
    return 0;
  }
  else if (options.find("--help") != options.end()) {
    std::cout << R"help(clang-querydb help:

  clang-querydb is a low-latency C++ language server.

  General:
    --help        Print this help information.
    --language-server
                  Run as a language server. The language server will look for
                  an existing querydb process, otherwise it will run querydb
                  in-process. This implements the language server spec.
    --querydb     Run the querydb. The querydb stores the program index and
                  serves index request tasks.
    --test        Run tests. Does nothing if test support is not compiled in.

  Configuration:
    When opening up a directory, clang-querydb will look for a
    compile_commands.json file emitted by your preferred build system. If not
    present, clang-querydb will use a recursive directory listing instead.
    Command line flags can be provided by adding a "clang_args" file in the
    top-level directory. Each line in that file is a separate argument.
)help";
    exit(0);
  }
  else if (HasOption(options, "--language-server")) {
    //std::cerr << "Running language server" << std::endl;
    LanguageServerMain(argv[0]);
    return 0;
  }
  else if (HasOption(options, "--querydb")) {
    //std::cerr << "Running querydb" << std::endl;
    QueryDbMain();
    return 0;
  }
  else {
    //std::cerr << "Running language server" << std::endl;
    LanguageServerMain(argv[0]);
    return 0;
  }

  return 1;
}
