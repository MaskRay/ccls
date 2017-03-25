// TODO: cleanup includes
#include "compilation_database_loader.h"
#include "indexer.h"
#include "query.h"
#include "language_server_api.h"
#include "platform.h"
#include "test.h"
#include "timer.h"
#include "threaded_queue.h"
#include "typed_bidi_message_queue.h"

#include <doctest/doctest.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <thread>
#include <vector>

namespace {
const char* kIpcLanguageClientName = "language_client";

const int kNumIndexers = 8 - 1;
const int kQueueSizeBytes = 1024 * 1024 * 32;
}

struct IndexTranslationUnitRequest {
  std::string path;
  std::vector<std::string> args;
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
  MessageRegistry::instance()->Register<Ipc_TextDocumentDocumentSymbol>();
  MessageRegistry::instance()->Register<Ipc_TextDocumentCodeLens>();
  MessageRegistry::instance()->Register<Ipc_CodeLensResolve>();
  MessageRegistry::instance()->Register<Ipc_WorkspaceSymbol>();
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

    // Parse request and send a response.
    std::cerr << "Parsing file " << request->path << " with args "
      << Join(request->args, ", ") << std::endl;

    Timer time;
    IndexedFile file = Parse(request->path, request->args);
    std::cerr << "Parsing/indexing took " << time.ElapsedMilliseconds()
      << "ms" << std::endl;

    time.Reset();
    IndexUpdate update(file);
    IndexTranslationUnitResponse response(update);
    std::cerr << "Creating index update took " << time.ElapsedMilliseconds()
      << "ms" << std::endl;

    time.Reset();
    responses->Enqueue(response);
    std::cerr << "Sending to server took " << time.ElapsedMilliseconds()
      << "ms" << std::endl;
  }
}

QueryableFile* FindFile(QueryableDatabase* db, const std::string& filename) {
  // std::cerr << "Wanted file " << msg->document << std::endl;
  // TODO: hashmap lookup.
  for (auto& file : db->files) {
    // std::cerr << " - Have file " << file.file_id << std::endl;
    if (file.file_id == filename) {
      std::cerr << "Found file " << filename << std::endl;
      return &file;
    }
  }

  std::cerr << "Unable to find file " << filename << std::endl;
  return nullptr;
}

lsLocation GetLsLocation(const QueryableLocation& location) {
  return lsLocation(
    lsDocumentUri::FromPath(location.path),
    lsRange(lsPosition(location.line - 1, location.column - 1)));
}

void AddCodeLens(std::vector<TCodeLens>* result,
  QueryableLocation loc,
  const std::vector<QueryableLocation>& uses,
  bool only_interesting,
  const char* singular,
  const char* plural) {
  TCodeLens code_lens;
  code_lens.range.start.line =
    loc.line - 1;  // TODO: cleanup indexer to negate by 1.
  code_lens.range.start.character =
    loc.column - 1;  // TODO: cleanup indexer to negate by 1.
                     // TODO: store range information.
  code_lens.range.end.line = code_lens.range.start.line;
  code_lens.range.end.character = code_lens.range.start.character;

  code_lens.command = lsCommand<lsCodeLensCommandArguments>();
  code_lens.command->command = "superindex.showReferences";
  code_lens.command->arguments.uri = lsDocumentUri::FromPath(loc.path);
  code_lens.command->arguments.position = code_lens.range.start;

  // Add unique uses.
  std::unordered_set<lsLocation> unique_uses;
  for (const QueryableLocation& use : uses) {
    if (only_interesting && !use.interesting)
      continue;
    unique_uses.insert(GetLsLocation(use));
  }
  code_lens.command->arguments.locations.assign(unique_uses.begin(),
    unique_uses.end());

  // User visible label
  int num_usages = unique_uses.size();
  code_lens.command->title = std::to_string(num_usages) + " ";
  if (num_usages == 1)
    code_lens.command->title += singular;
  else
    code_lens.command->title += plural;

  if (unique_uses.size() > 0)
    result->push_back(code_lens);
}

void AddCodeLens(std::vector<TCodeLens>* result,
  QueryableLocation loc,
  const std::vector<UsrRef>& uses,
  bool only_interesting,
  const char* singular,
  const char* plural) {
  std::vector<QueryableLocation> uses0;
  uses0.reserve(uses.size());
  for (const UsrRef& use : uses)
    uses0.push_back(use.loc);
  AddCodeLens(result, loc, uses0, only_interesting, singular, plural);
}

void AddCodeLens(std::vector<TCodeLens>* result,
  QueryableDatabase* db,
  QueryableLocation loc,
  const std::vector<Usr>& usrs,
  bool only_interesting,
  const char* singular,
  const char* plural) {
  std::vector<QueryableLocation> uses0;
  uses0.reserve(usrs.size());
  for (const Usr& usr : usrs) {
    SymbolIdx symbol = db->usr_to_symbol[usr];
    switch (symbol.kind) {
    case SymbolKind::Type: {
      QueryableTypeDef* def = &db->types[symbol.idx];
      if (def->def.definition)
        uses0.push_back(def->def.definition.value());
      break;
    }
    case SymbolKind::Func: {
      QueryableFuncDef* def = &db->funcs[symbol.idx];
      if (def->def.definition)
        uses0.push_back(def->def.definition.value());
      break;
    }
    case SymbolKind::Var: {
      QueryableVarDef* def = &db->vars[symbol.idx];
      if (def->def.definition)
        uses0.push_back(def->def.definition.value());
      break;
    }
    case SymbolKind::File:
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
    }
  }
  AddCodeLens(result, loc, uses0, only_interesting, singular, plural);
}

void QueryDbMainLoop(
  QueryableDatabase* db,
  IpcMessageQueue* language_client,
  IndexRequestQueue* index_requests,
  IndexResponseQueue* index_responses) {
  std::vector<std::unique_ptr<BaseIpcMessage>> messages = language_client->GetMessages(&language_client->for_server);
  for (auto& message : messages) {
    // std::cerr << "Processing message " << static_cast<int>(message->ipc_id)
    // << std::endl;

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

      std::vector<CompilationEntry> entries =
        LoadCompilationEntriesFromDirectory(path);
      for (int i = 0; i < entries.size(); ++i) {
        const CompilationEntry& entry = entries[i];
        std::string filepath = path + "/" + entry.filename;
        std::cerr << "[" << i << "/" << (entries.size() - 1)
          << "] Dispatching index request for file " << filepath
          << std::endl;

        IndexTranslationUnitRequest request;
        request.path = filepath;
        request.args = entry.args;
        index_requests->Enqueue(request);
      }
      std::cerr << "Done" << std::endl;
      break;
    }

    case IpcId::TextDocumentDocumentSymbol: {
      auto msg = static_cast<Ipc_TextDocumentDocumentSymbol*>(message.get());

      Out_TextDocumentDocumentSymbol response;
      response.id = msg->id;

      QueryableFile* file = FindFile(db, msg->params.textDocument.uri.GetPath());
      if (file) {
        for (UsrRef ref : file->outline) {
          SymbolIdx symbol = db->usr_to_symbol[ref.usr];

          lsSymbolInformation info;
          info.location.range.start.line =
            ref.loc.line - 1;  // TODO: cleanup indexer to negate by 1.
          info.location.range.start.character =
            ref.loc.column - 1;  // TODO: cleanup indexer to negate by 1.
                                 // TODO: store range information.
          info.location.range.end.line = info.location.range.start.line;
          info.location.range.end.character =
            info.location.range.start.character;

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
      if (file) {
        for (UsrRef ref : file->outline) {
          SymbolIdx symbol = db->usr_to_symbol[ref.usr];
          switch (symbol.kind) {
          case SymbolKind::Type: {
            QueryableTypeDef& def = db->types[symbol.idx];
            AddCodeLens(&response.result, ref.loc, def.uses,
              true /*only_interesting*/, "reference",
              "references");
            AddCodeLens(&response.result, db, ref.loc, def.derived,
              false /*only_interesting*/, "derived", "derived");
            break;
          }
          case SymbolKind::Func: {
            QueryableFuncDef& def = db->funcs[symbol.idx];
            AddCodeLens(&response.result, ref.loc, def.uses,
              false /*only_interesting*/, "reference",
              "references");
            AddCodeLens(&response.result, ref.loc, def.callers,
              false /*only_interesting*/, "caller", "callers");
            AddCodeLens(&response.result, ref.loc, def.def.callees,
              false /*only_interesting*/, "callee", "callees");
            AddCodeLens(&response.result, db, ref.loc, def.derived,
              false /*only_interesting*/, "derived", "derived");
            break;
          }
          case SymbolKind::Var: {
            QueryableVarDef& def = db->vars[symbol.idx];
            AddCodeLens(&response.result, ref.loc, def.uses,
              false /*only_interesting*/, "reference",
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

            if (def.def.definition.has_value()) {
              info.location.uri.SetPath(def.def.definition->path);
              info.location.range.start.line = def.def.definition->line - 1;
              info.location.range.start.character =
                def.def.definition->column - 1;
            }
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

            if (def.def.definition.has_value()) {
              info.location.uri.SetPath(def.def.definition->path);
              info.location.range.start.line = def.def.definition->line - 1;
              info.location.range.start.character =
                def.def.definition->column - 1;
            }
            break;
          }
          case SymbolKind::Var: {
            QueryableVarDef& def = db->vars[symbol.idx];
            info.name = def.def.qualified_name;
            info.kind = lsSymbolKind::Variable;

            if (def.def.definition.has_value()) {
              info.location.uri.SetPath(def.def.definition->path);
              info.location.range.start.line = def.def.definition->line - 1;
              info.location.range.start.character =
                def.def.definition->column - 1;
            }
            break;
          }
          case SymbolKind::Invalid: {
            assert(false && "unexpected");
            break;
          }
          };

          // TODO: store range information.
          info.location.range.end.line = info.location.range.start.line;
          info.location.range.end.character =
            info.location.range.start.character;

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
    std::cerr << "Applying index update took " << time.ElapsedMilliseconds()
      << "ms" << std::endl;
  }
}

void QueryDbMain() {
  std::cerr << "Running QueryDb" << std::endl;

  // Create queues.
  std::unique_ptr<IpcMessageQueue> ipc = BuildIpcMessageQueue(kIpcLanguageClientName, kQueueSizeBytes);
  IndexRequestQueue index_request_queue;
  IndexResponseQueue index_response_queue;

  // Start indexer threads.
  for (int i = 0; i < kNumIndexers; ++i) {
    new std::thread([&]() {
      IndexMain(&index_request_queue, &index_response_queue);
    });
  }

  // Run query db main loop.
  QueryableDatabase db;
  while (true) {
    QueryDbMainLoop(&db, ipc.get(), &index_request_queue, &index_response_queue);
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

    std::cerr << "[info]: Got message of type "
      << IpcIdToString(message->method_id) << std::endl;
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

      auto response = Out_InitializeResponse();
      response.id = request->id;
      response.result.capabilities.documentSymbolProvider = true;
      // response.result.capabilities.referencesProvider = true;
      response.result.capabilities.codeLensProvider = lsCodeLensOptions();
      response.result.capabilities.codeLensProvider->resolveProvider = false;
      response.result.capabilities.workspaceSymbolProvider = true;
      response.Write(std::cout);
      break;
    }

    case IpcId::TextDocumentDocumentSymbol:
    case IpcId::TextDocumentCodeLens:
    case IpcId::WorkspaceSymbol: {
      ipc->SendMessage(&ipc->for_server, message->method_id, *message.get());
      break;
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
  bool loop = false;
  while (loop)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

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
