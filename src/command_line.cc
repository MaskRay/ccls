// TODO: cleanup includes
#include "code_completion.h"
#include "indexer.h"
#include "query.h"
#include "language_server_api.h"
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
}

struct IndexTranslationUnitRequest {
  enum class Type {
    Import,
    Update
  };

  std::string path;
  std::vector<std::string> args;
  Type type;

  IndexTranslationUnitRequest(Type type) : type(type) {}
};

struct IndexTranslationUnitResponse {
  IndexUpdate update;
  explicit IndexTranslationUnitResponse(IndexUpdate& update) : update(update) {}
};

// TODO: Rename TypedBidiMessageQueue to IpcTransport?
using IpcMessageQueue = TypedBidiMessageQueue<IpcId, BaseIpcMessage>;
using IndexRequestQueue = ThreadedQueue<IndexTranslationUnitRequest>;
using IndexResponseQueue = ThreadedQueue<IndexTranslationUnitResponse>;

template<typename TMessage>
void SendMessage(IpcMessageQueue& t, MessageQueue* destination, TMessage& message) {
  t.SendMessage(destination, TMessage::kIpcId, message);
}

std::unordered_map<std::string, std::string> ParseOptions(int argc,
  char** argv) {
  std::unordered_map<std::string, std::string> output;

  std::string previous_arg;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg[0] != '-') {
      if (previous_arg.size() == 0) {
        std::cerr << "Invalid arguments; switches must start with -"
          << std::endl;
        exit(1);
      }

      output[previous_arg] = arg;
      previous_arg = "";
    }
    else {
      output[arg] = "";
      previous_arg = arg;
    }
  }

  return output;
}

bool HasOption(const std::unordered_map<std::string, std::string>& options,
  const std::string& option) {
  return options.find(option) != options.end();
}


std::string Join(const std::vector<std::string>& elements, std::string sep) {
  bool first = true;
  std::string result;
  for (const auto& element : elements) {
    if (!first)
      result += ", ";
    first = false;
    result += element;
  }
  return result;
}


optional<QueryableLocation> GetDefinitionSpellingOfUsr(QueryableDatabase* db, const Usr& usr) {
  SymbolIdx symbol = db->usr_to_symbol[usr];
  switch (symbol.kind) {
  case SymbolKind::Type: {
    QueryableTypeDef* def = &db->types[symbol.idx];
    return def->def.definition_spelling;
  }
  case SymbolKind::Func: {
    QueryableFuncDef* def = &db->funcs[symbol.idx];
    return def->def.definition_spelling;
  }
  case SymbolKind::Var: {
    QueryableVarDef* def = &db->vars[symbol.idx];
    return def->def.definition_spelling;
  }
  case SymbolKind::File:
  case SymbolKind::Invalid: {
    assert(false && "unexpected");
    break;
  }
  }
  return nullopt;
}

optional<QueryableLocation> GetDefinitionExtentOfUsr(QueryableDatabase* db, const Usr& usr) {
  SymbolIdx symbol = db->usr_to_symbol[usr];
  switch (symbol.kind) {
  case SymbolKind::Type: {
    QueryableTypeDef* def = &db->types[symbol.idx];
    return def->def.definition_extent;
  }
  case SymbolKind::Func: {
    QueryableFuncDef* def = &db->funcs[symbol.idx];
    return def->def.definition_extent;
  }
  case SymbolKind::Var: {
    QueryableVarDef* def = &db->vars[symbol.idx];
    return def->def.definition_extent;
  }
  case SymbolKind::File:
  case SymbolKind::Invalid: {
    assert(false && "unexpected");
    break;
  }
  }
  return nullopt;
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
  RegisterId<Ipc_TextDocumentComplete>(ipc.get());
  RegisterId<Ipc_TextDocumentDefinition>(ipc.get());
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
  MessageRegistry::instance()->Register<Ipc_TextDocumentComplete>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDefinition>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentDocumentSymbol>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentCodeLens>();
  MessageRegistry::instance()->Register<Ipc_CodeLensResolve>();
  MessageRegistry::instance()->Register<Ipc_WorkspaceSymbol>();
}

std::string GetCachedFileName(std::string source_file) {
  // TODO/FIXME
  const char* kCacheDirectory = "C:/Users/jacob/Desktop/superindex/indexer/CACHE/";
  std::replace(source_file.begin(), source_file.end(), '\\', '_');
  std::replace(source_file.begin(), source_file.end(), '/', '_');
  std::replace(source_file.begin(), source_file.end(), ':', '_');
  std::replace(source_file.begin(), source_file.end(), '.', '_');
  return kCacheDirectory + source_file + ".json";
}

optional<IndexedFile> LoadCachedFile(std::string filename) {
  // TODO FIXME FIXME FIXME
  return nullopt;

  std::string cache_file = GetCachedFileName(filename);

  std::ifstream cache;
  cache.open(GetCachedFileName(filename));
  if (!cache.good())
    return nullopt;

  std::string file_content = std::string(
    std::istreambuf_iterator<char>(cache),
    std::istreambuf_iterator<char>());

  return Deserialize(filename, file_content);
}

void WriteToCache(std::string filename, IndexedFile& file) {
  std::string indexed_content = Serialize(file);

  std::ofstream cache;
  cache.open(GetCachedFileName(filename));
  assert(cache.good());
  cache << indexed_content;
  cache.close();
}

void IndexMain(IndexRequestQueue* requests, IndexResponseQueue* responses) {
  while (true) {
    // Try to get a request. If there isn't one, sleep for a little while.
    optional<IndexTranslationUnitRequest> request = requests->TryDequeue();
    if (!request) {
      // TODO: use CV to wakeup?
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      continue;
    }


    Timer time;

    // If the index update is an import, then we will load the previous index
    // into memory if we have a previous index. After that, we dispatch an
    // update request to get the latest version.
    if (request->type == IndexTranslationUnitRequest::Type::Import) {
      request->type = IndexTranslationUnitRequest::Type::Update;

      // TODO: we're not serializing out the files cache. We only ever want to import references
      // from the primary file though, so that should be ok. We need to cleanup indexer output.
      optional<IndexedFile> old_index = LoadCachedFile(request->path);
      if (old_index.has_value()) {
        IndexUpdate update = IndexUpdate::CreateImport(old_index.value());
        IndexTranslationUnitResponse response(update);
        responses->Enqueue(response);
        time.ResetAndPrint("Loading cached index");
        requests->Enqueue(request.value());
        continue;
      }
    }

    assert(request->type == IndexTranslationUnitRequest::Type::Update);

    // Parse request and send a response.
    std::cerr << "Parsing file " << request->path << " with args "
      << Join(request->args, ", ") << std::endl;

    IndexedFile new_index = Parse(request->path, request->args);
    time.ResetAndPrint("Parsing/indexing");

    // If we have a cached index, that means it is already imported, which
    // means we want to apply a delta update.
    optional<IndexedFile> old_index = LoadCachedFile(request->path);
    time.ResetAndPrint("Loading previous index");
    if (old_index) {
      // Apply delta update.
      IndexUpdate update = IndexUpdate::CreateDelta(old_index.value(), new_index);
      IndexTranslationUnitResponse response(update);
      time.ResetAndPrint("Creating delta index update/response");
      responses->Enqueue(response);
      time.ResetAndPrint("Sending update to server");
    }
    else {
      // Apply full update.
      IndexUpdate update = IndexUpdate::CreateImport(new_index);
      IndexTranslationUnitResponse response(update);
      time.ResetAndPrint("Creating index update/response");
      responses->Enqueue(response);
      time.ResetAndPrint("Sending update to server");
    }

    // Cache file so we can diff it later.
    WriteToCache(request->path, new_index);
    time.ResetAndPrint("Cache index update to disk");
  }
}

QueryableFile* FindFile(QueryableDatabase* db, const std::string& filename) {
  // std::cerr << "Wanted file " << msg->document << std::endl;
  // TODO: hashmap lookup.
  for (auto& file : db->files) {
    // std::cerr << " - Have file " << file.file_id << std::endl;
    if (file.def.usr == filename) {
      //std::cerr << "Found file " << filename << std::endl;
      return &file;
    }
  }

  std::cerr << "Unable to find file " << filename << std::endl;
  return nullptr;
}

lsRange GetLsRange(const Range& location) {
  return lsRange(
      lsPosition(location.start.line - 1, location.start.column - 1),
      lsPosition(location.end.line - 1, location.end.column - 1));
}

lsLocation GetLsLocation(const QueryableLocation& location) {
  return lsLocation(
    lsDocumentUri::FromPath(location.path),
    GetLsRange(location.range));
}

void AddCodeLens(std::vector<TCodeLens>* result,
  QueryableLocation loc,
  const std::vector<QueryableLocation>& uses,
  bool exclude_loc,
  bool only_interesting,
  const char* singular,
  const char* plural) {
  TCodeLens code_lens;
  code_lens.range = GetLsRange(loc.range);
  code_lens.command = lsCommand<lsCodeLensCommandArguments>();
  code_lens.command->command = "superindex.showReferences";
  code_lens.command->arguments.uri = lsDocumentUri::FromPath(loc.path);
  code_lens.command->arguments.position = code_lens.range.start;

  // Add unique uses.
  std::unordered_set<lsLocation> unique_uses;
  for (const QueryableLocation& use : uses) {
    if (exclude_loc && use == loc)
      continue;
    if (only_interesting && !use.range.interesting)
      continue;
    unique_uses.insert(GetLsLocation(use));
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

void AddCodeLens(std::vector<TCodeLens>* result,
  QueryableLocation loc,
  const std::vector<UsrRef>& uses,
  bool exclude_loc,
  bool only_interesting,
  const char* singular,
  const char* plural) {
  std::vector<QueryableLocation> uses0;
  uses0.reserve(uses.size());
  for (const UsrRef& use : uses)
    uses0.push_back(use.loc);
  AddCodeLens(result, loc, uses0, exclude_loc, only_interesting, singular, plural);
}

void AddCodeLens(std::vector<TCodeLens>* result,
  QueryableDatabase* db,
  QueryableLocation loc,
  const std::vector<Usr>& usrs,
  bool exclude_loc,
  bool only_interesting,
  const char* singular,
  const char* plural) {
  std::vector<QueryableLocation> uses0;
  uses0.reserve(usrs.size());
  for (const Usr& usr : usrs) {
    optional<QueryableLocation> loc = GetDefinitionSpellingOfUsr(db, usr);
    if (loc)
      uses0.push_back(loc.value());
  }
  AddCodeLens(result, loc, uses0, exclude_loc, only_interesting, singular, plural);
}

void QueryDbMainLoop(
  QueryableDatabase* db,
  IpcMessageQueue* language_client,
  IndexRequestQueue* index_requests,
  IndexResponseQueue* index_responses,
  Project* project,
  WorkingFiles* working_files,
  CompletionManager* completion_manager) {

  std::vector<std::unique_ptr<BaseIpcMessage>> messages = language_client->GetMessages(&language_client->for_server);
  for (auto& message : messages) {
    std::cerr << "[querydb] Processing message " << static_cast<int>(message->method_id) << std::endl;

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

        IndexTranslationUnitRequest request(IndexTranslationUnitRequest::Type::Import);
        request.path = filepath;
        request.args = entry.args;
        index_requests->Enqueue(request);
      }
      std::cerr << "Done" << std::endl;
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
      //working_files->OnClose(msg->params);
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

      // TODO: Edge cases (whitespace, etc) will work a lot better
      // if we store range information instead of hacking it.
      int target_line = msg->params.position.line + 1;
      int target_column = msg->params.position.character + 1;

      for (const UsrRef& ref : file->def.all_symbols) {
        if (ref.loc.range.start.line >= target_line && ref.loc.range.end.line <= target_line &&
            ref.loc.range.start.column <= target_column && ref.loc.range.end.column >= target_column) {
          optional<QueryableLocation> location = GetDefinitionSpellingOfUsr(db, ref.usr);
          if (location)
            response.result.push_back(GetLsLocation(location.value()));
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
      for (UsrRef ref : file->def.outline) {
        SymbolIdx symbol = db->usr_to_symbol[ref.usr];

        lsSymbolInformation info;
        info.location = GetLsLocation(ref.loc);

        // TODO: cleanup namespace/naming so there is only one SymbolKind.
        switch (symbol.kind) {
        case SymbolKind::Type: {
          QueryableTypeDef& def = db->types[symbol.idx];
          info.name = def.def.qualified_name;
          info.kind = lsSymbolKind::Class;
          break;
        }
        case SymbolKind::Func: {
          QueryableFuncDef& def = db->funcs[symbol.idx];
          info.name = def.def.qualified_name;
          if (def.def.declaring_type.has_value()) {
            info.kind = lsSymbolKind::Method;
            Usr declaring = def.def.declaring_type.value();
            info.containerName =
              db->types[db->usr_to_symbol[declaring].idx]
              .def.qualified_name;
          }
          else {
            info.kind = lsSymbolKind::Function;
          }
          break;
        }
        case SymbolKind::Var: {
          QueryableVarDef& def = db->vars[symbol.idx];
          info.name = def.def.qualified_name;
          info.kind = lsSymbolKind::Variable;
          break;
        }
        case SymbolKind::File:
        case SymbolKind::Invalid: {
          assert(false && "unexpected");
          break;
        }
        };

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

      for (UsrRef ref : file->def.outline) {
        // NOTE: We OffsetColumn so that the code lens always show up in a
        // predictable order. Otherwise, the client may randomize it.

        SymbolIdx symbol = db->usr_to_symbol[ref.usr];
        switch (symbol.kind) {
        case SymbolKind::Type: {
          QueryableTypeDef& def = db->types[symbol.idx];
          AddCodeLens(&response.result, ref.loc.OffsetStartColumn(0), def.uses,
            false /*exclude_loc*/, false /*only_interesting*/, "ref",
            "refs");
          AddCodeLens(&response.result, ref.loc.OffsetStartColumn(1), def.uses,
            false /*exclude_loc*/, true /*only_interesting*/, "iref",
            "irefs");
          AddCodeLens(&response.result, db, ref.loc.OffsetStartColumn(2), def.derived,
            false /*exclude_loc*/, false /*only_interesting*/, "derived", "derived");
          AddCodeLens(&response.result, db, ref.loc.OffsetStartColumn(3), def.instantiations,
            false /*exclude_loc*/, false /*only_interesting*/, "instantiation", "instantiations");
          break;
        }
        case SymbolKind::Func: {
          QueryableFuncDef& def = db->funcs[symbol.idx];
          //AddCodeLens(&response.result, ref.loc.OffsetStartColumn(0), def.uses,
          //  false /*exclude_loc*/, false /*only_interesting*/, "reference",
          //  "references");
          AddCodeLens(&response.result, ref.loc.OffsetStartColumn(1), def.callers,
            true /*exclude_loc*/, false /*only_interesting*/, "caller", "callers");
          //AddCodeLens(&response.result, ref.loc.OffsetColumn(2), def.def.callees,
          //  false /*exclude_loc*/, false /*only_interesting*/, "callee", "callees");
          AddCodeLens(&response.result, db, ref.loc.OffsetStartColumn(3), def.derived,
            false /*exclude_loc*/, false /*only_interesting*/, "derived", "derived");
          break;
        }
        case SymbolKind::Var: {
          QueryableVarDef& def = db->vars[symbol.idx];
          AddCodeLens(&response.result, ref.loc.OffsetStartColumn(0), def.uses,
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
        const std::string& name = db->qualified_names[i];
        // std::cerr << "- Considering " << name << std::endl;

        if (name.find(query) != std::string::npos) {
          lsSymbolInformation info;
          info.name = name;

          SymbolIdx symbol = db->symbols[i];

          // TODO: dedup this code w/ above (ie, add ctor to convert symbol to
          // SymbolInformation)
          switch (symbol.kind) {
            // TODO: file
          case SymbolKind::Type: {
            QueryableTypeDef& def = db->types[symbol.idx];
            info.name = def.def.qualified_name;
            info.kind = lsSymbolKind::Class;

            if (def.def.definition_extent.has_value())
              info.location = GetLsLocation(def.def.definition_extent.value());
            break;
          }
          case SymbolKind::Func: {
            QueryableFuncDef& def = db->funcs[symbol.idx];
            info.name = def.def.qualified_name;
            if (def.def.declaring_type.has_value()) {
              info.kind = lsSymbolKind::Method;
              Usr declaring = def.def.declaring_type.value();
              info.containerName =
                db->types[db->usr_to_symbol[declaring].idx]
                .def.qualified_name;
            }
            else {
              info.kind = lsSymbolKind::Function;
            }

            if (def.def.definition_extent.has_value()) {
              info.location = GetLsLocation(def.def.definition_extent.value());
            }
            break;
          }
          case SymbolKind::Var: {
            QueryableVarDef& def = db->vars[symbol.idx];
            info.name = def.def.qualified_name;
            info.kind = lsSymbolKind::Variable;

            if (def.def.definition_extent.has_value()) {
              info.location = GetLsLocation(def.def.definition_extent.value());
            }
            break;
          }
          case SymbolKind::Invalid: {
            assert(false && "unexpected");
            break;
          }
          };

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
    optional<IndexTranslationUnitResponse> response = index_responses->TryDequeue();
    if (!response)
      break;

    Timer time;
    db->ApplyIndexUpdate(&response->update);
    time.ResetAndPrint("Applying index update");
  }
}

void QueryDbMain() {
  std::cerr << "Running QueryDb" << std::endl;

  // Create queues.
  std::unique_ptr<IpcMessageQueue> ipc = BuildIpcMessageQueue(kIpcLanguageClientName, kQueueSizeBytes);
  IndexRequestQueue index_request_queue;
  IndexResponseQueue index_response_queue;
  Project project;
  WorkingFiles working_files;
  CompletionManager completion_manager(&project, &working_files);

  // Start indexer threads.
  for (int i = 0; i < kNumIndexers; ++i) {
    new std::thread([&]() {
      IndexMain(&index_request_queue, &index_response_queue);
    });
  }

  // Run query db main loop.
  QueryableDatabase db;
  while (true) {
    QueryDbMainLoop(&db, ipc.get(), &index_request_queue, &index_response_queue, &project, &working_files, &completion_manager);
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
      response.result.capabilities.textDocumentSync = lsTextDocumentSyncKind::Full; // TODO: use incremental at some point

      response.result.capabilities.completionProvider = lsCompletionOptions();
      response.result.capabilities.completionProvider->resolveProvider = false;
      response.result.capabilities.completionProvider->triggerCharacters = { ".", "::", "->" };

      response.result.capabilities.codeLensProvider = lsCodeLensOptions();
      response.result.capabilities.codeLensProvider->resolveProvider = false;

      response.result.capabilities.definitionProvider = true;

      response.result.capabilities.documentSymbolProvider = true;

      response.result.capabilities.workspaceSymbolProvider = true;

      response.Write(std::cerr);
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
    case IpcId::TextDocumentDidClose: {
    case IpcId::TextDocumentCompletion:
    case IpcId::TextDocumentDefinition:
    case IpcId::TextDocumentDocumentSymbol:
    case IpcId::TextDocumentCodeLens:
    case IpcId::WorkspaceSymbol:
      std::cerr << "Spending message " << (int)message->method_id << std::endl;
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
    std::cerr << "Running language server" << std::endl;
    LanguageServerMain(argv[0]);
    return 0;
  }
  else if (HasOption(options, "--querydb")) {
    std::cerr << "Running querydb" << std::endl;
    QueryDbMain();
    return 0;
  }
  else {
    std::cerr << "Running language server" << std::endl;
    LanguageServerMain(argv[0]);
    return 0;
  }

  return 1;
}
