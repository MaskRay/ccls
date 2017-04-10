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

#include <doctest/doctest.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <thread>
#include <vector>

namespace {

const char* kIpcLanguageClientName = "language_client";

const int kNumIndexers = 8 - 1;
const int kQueueSizeBytes = 1024 * 8;
const int kMaxWorkspaceSearchResults = 1000;

QueryableFile* FindFile(QueryableDatabase* db, const std::string& filename) {
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

std::vector<QueryableLocation> GetUsesOfSymbol(QueryableDatabase* db, const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type:
      return db->types[symbol.idx].uses;
    case SymbolKind::Func:
      return db->funcs[symbol.idx].uses;
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
      info.name = def->def.qualified_name;
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


void AddCodeLens(
  QueryableDatabase* db,
  WorkingFiles* working_files,
  std::vector<TCodeLens>* result,
  QueryableLocation loc,
  WorkingFile* working_file,
  const std::vector<QueryableLocation>& uses,
  bool exclude_loc,
  bool only_interesting,
  const char* singular,
  const char* plural) {
  TCodeLens code_lens;
  optional<lsRange> range = GetLsRange(working_file, loc.range);
  if (!range)
    return;
  code_lens.range = *range;
  code_lens.command = lsCommand<lsCodeLensCommandArguments>();
  code_lens.command->command = "superindex.showReferences";
  code_lens.command->arguments.uri = GetLsDocumentUri(db, loc.path);
  code_lens.command->arguments.position = code_lens.range.start;

  // Add unique uses.
  std::unordered_set<lsLocation> unique_uses;
  for (const QueryableLocation& use : uses) {
    if (exclude_loc && use == loc)
      continue;
    if (only_interesting && !use.range.interesting)
      continue;
    optional<lsLocation> location = GetLsLocation(db, working_files, use);
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
    result->push_back(code_lens);
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
  RegisterId<Ipc_TextDocumentComplete>(ipc.get());
  RegisterId<Ipc_TextDocumentDefinition>(ipc.get());
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
  MessageRegistry::instance()->Register<Ipc_TextDocumentComplete>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDefinition>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentReferences>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDocumentSymbol>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentCodeLens>();
  MessageRegistry::instance()->Register<Ipc_CodeLensResolve>();
  MessageRegistry::instance()->Register<Ipc_WorkspaceSymbol>();
}

bool IndexMain_DoIndex(FileConsumer* file_consumer,
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
    time.ResetAndPrint("Loading cached index " + index_request->path);

    // If import fails just do a standard update.
    if (old_index) {
      Index_DoIdMap response(nullptr, std::move(old_index));
      queue_do_id_map->Enqueue(std::move(response));

      queue_do_index->Enqueue(std::move(*index_request));
      return true;
    }
  }

  // Parse request and send a response.
  std::vector<std::unique_ptr<IndexedFile>> indexes = Parse(file_consumer, index_request->path, index_request->args);
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

bool IndexMain_DoCreateIndexUpdate(Index_OnIdMappedQueue* queue_on_id_mapped,
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

void IndexMain(
  FileConsumer::SharedState* file_consumer_shared,
  Index_DoIndexQueue* queue_do_index,
  Index_DoIdMapQueue* queue_do_id_map,
  Index_OnIdMappedQueue* queue_on_id_mapped,
  Index_OnIndexedQueue* queue_on_indexed) {

  FileConsumer file_consumer(file_consumer_shared);

  while (true) {
    // TODO: process all off IndexMain_DoIndex before calling IndexMain_DoCreateIndexUpdate for
    //       better icache behavior. We need to have some threads spinning on both though
    //       otherwise memory usage will get bad.

    if (!IndexMain_DoIndex(&file_consumer, queue_do_index, queue_do_id_map) &&
      !IndexMain_DoCreateIndexUpdate(queue_on_id_mapped, queue_on_indexed)) {
      // TODO: use CV to wakeup?
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
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

      QueryableFile* file = FindFile(db, msg->params.textDocument.uri.GetPath());
      if (!file) {
        std::cerr << "Unable to find file " << msg->params.textDocument.uri.GetPath() << std::endl;
        break;
      }

      Out_TextDocumentDefinition response;
      response.id = msg->id;

      int target_line = msg->params.position.line + 1;
      int target_column = msg->params.position.character + 1;

      for (const SymbolRef& ref : file->def.all_symbols) {
        if (ref.loc.range.start.line >= target_line && ref.loc.range.end.line <= target_line &&
            ref.loc.range.start.column <= target_column && ref.loc.range.end.column >= target_column) {
          // Found symbol. Return definition.
          optional<QueryableLocation> location = GetDefinitionSpellingOfSymbol(db, ref.idx);
          if (location) {
            optional<lsLocation> ls_location = GetLsLocation(db, working_files, location.value());
            if (ls_location)
              response.result.push_back(*ls_location);
          }
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

      // TODO: Edge cases (whitespace, etc) will work a lot better
      // if we store range information instead of hacking it.
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
      WorkingFile* working_file = working_files->GetFileByFilename(file->def.usr);

      for (SymbolRef ref : file->def.outline) {
        // NOTE: We OffsetColumn so that the code lens always show up in a
        // predictable order. Otherwise, the client may randomize it.

        SymbolIdx symbol = ref.idx;
        switch (symbol.kind) {
        case SymbolKind::Type: {
          QueryableTypeDef& def = db->types[symbol.idx];
          AddCodeLens(db, working_files, &response.result, ref.loc.OffsetStartColumn(0), working_file, def.uses,
            false /*exclude_loc*/, false /*only_interesting*/, "ref",
            "refs");
          AddCodeLens(db, working_files, &response.result, ref.loc.OffsetStartColumn(1), working_file, def.uses,
            false /*exclude_loc*/, true /*only_interesting*/, "iref",
            "irefs");
          AddCodeLens(db, working_files, &response.result, ref.loc.OffsetStartColumn(2), working_file, ToQueryableLocation(db, def.derived),
            false /*exclude_loc*/, false /*only_interesting*/, "derived", "derived");
          AddCodeLens(db, working_files, &response.result, ref.loc.OffsetStartColumn(3), working_file, ToQueryableLocation(db, def.instantiations),
            false /*exclude_loc*/, false /*only_interesting*/, "instantiation", "instantiations");
          break;
        }
        case SymbolKind::Func: {
          QueryableFuncDef& def = db->funcs[symbol.idx];
          //AddCodeLens(&response.result, ref.loc.OffsetStartColumn(0), def.uses,
          //  false /*exclude_loc*/, false /*only_interesting*/, "reference",
          //  "references");
          AddCodeLens(db, working_files, &response.result, ref.loc.OffsetStartColumn(1), working_file, ToQueryableLocation(db, def.callers),
            true /*exclude_loc*/, false /*only_interesting*/, "caller", "callers");
          //AddCodeLens(&response.result, ref.loc.OffsetColumn(2), def.def.callees,
          //  false /*exclude_loc*/, false /*only_interesting*/, "callee", "callees");
          AddCodeLens(db, working_files, &response.result, ref.loc.OffsetStartColumn(3), working_file, ToQueryableLocation(db, def.derived),
            false /*exclude_loc*/, false /*only_interesting*/, "derived", "derived");
          break;
        }
        case SymbolKind::Var: {
          QueryableVarDef& def = db->vars[symbol.idx];
          AddCodeLens(db, working_files, &response.result, ref.loc.OffsetStartColumn(0), working_file, def.uses,
            true /*exclude_loc*/, false /*only_interesting*/, "reference",
            "references");
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
        << " candidates " << std::endl;

      std::string query = msg->params.query;
      for (int i = 0; i < db->qualified_names.size(); ++i) {
        if (response.result.size() > kMaxWorkspaceSearchResults) {
          std::cerr << "Query exceeded maximum number of responses (" << kMaxWorkspaceSearchResults << "), output may not contain all results" << std::endl;
          break;
        }

        if (db->qualified_names[i].find(query) != std::string::npos) {
          lsSymbolInformation info = GetSymbolInfo(db, working_files, db->symbols[i]);
          optional<QueryableLocation> location = GetDefinitionExtentOfSymbol(db, db->symbols[i]);
          if (!location)
            continue;
          optional<lsLocation> ls_location = GetLsLocation(db, working_files, *location);
          if (!ls_location)
            continue;
          info.location = *ls_location;
          response.result.push_back(info);
        }
      }

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

      response.result.capabilities.completionProvider = lsCompletionOptions();
      response.result.capabilities.completionProvider->resolveProvider = false;
      response.result.capabilities.completionProvider->triggerCharacters = { ".", "::", "->" };

      response.result.capabilities.codeLensProvider = lsCodeLensOptions();
      response.result.capabilities.codeLensProvider->resolveProvider = false;

      response.result.capabilities.definitionProvider = true;
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
    case IpcId::TextDocumentCompletion:
    case IpcId::TextDocumentDefinition:
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
  //bool loop = true;
  //while (loop)
  //  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  std::this_thread::sleep_for(std::chrono::seconds(3));

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
