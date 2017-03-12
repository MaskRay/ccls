#include <iostream>
#include <string>
#include <unordered_map>
#include <thread>
#include <vector>

#include "compilation_database_loader.h"
#include "indexer.h"
#include "ipc.h"
#include "query.h"
#include "language_server_api.h"
#include "test.h"

#include "third_party/tiny-process-library/process.hpp"

#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

std::unordered_map<std::string, std::string> ParseOptions(int argc, char** argv) {
  std::unordered_map<std::string, std::string> output;

  std::string previous_arg;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg[0] != '-') {
      if (previous_arg.size() == 0) {
        std::cerr << "Invalid arguments; switches must start with -" << std::endl;
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

bool HasOption(const std::unordered_map<std::string, std::string>& options, const std::string& option) {
  return options.find(option) != options.end();
}



std::unique_ptr<InMessage> ParseMessage() {
  int content_length = -1;
  int iteration = 0;
  while (true) {
    if (++iteration > 10) {
      assert(false && "bad parser state");
      exit(1);
    }

    std::string line;
    std::getline(std::cin, line);
    //std::cin >> line;

    if (line.compare(0, 14, "Content-Length") == 0) {
      content_length = atoi(line.c_str() + 16);
    }

    if (line == "\r")
      break;
  }

  // bad input that is not a message.
  if (content_length < 0) {
    std::cerr << "parsing command failed (no Content-Length header)" << std::endl;
    return nullptr;
  }

  std::string content;
  content.reserve(content_length);
  for (int i = 0; i < content_length; ++i) {
    char c;
    std::cin >> c;
    content += c;
  }

  rapidjson::Document document;
  document.Parse(content.c_str(), content_length);
  assert(!document.HasParseError());

  return MessageRegistry::instance()->Parse(document);
}








































template<typename T>
struct BaseIpcMessage : public IpcMessage {
  BaseIpcMessage() : IpcMessage(T::kIpcId) {}

  // IpcMessage:
  void Serialize(Writer& writer) override {
    T& value = *static_cast<T*>(this);
    Reflect(writer, value);
  }
  void Deserialize(Reader& reader) override {
    T& value = *static_cast<T*>(this);
    Reflect(reader, value);
  }
};


struct IpcMessage_Quit : public BaseIpcMessage<IpcMessage_Quit> {
  static constexpr IpcId kIpcId = IpcId::Quit;
};
template<typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_Quit& value) {}


struct IpcMessage_IsAlive : public BaseIpcMessage<IpcMessage_IsAlive> {
  static constexpr IpcId kIpcId = IpcId::IsAlive;
};
template<typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_IsAlive& value) {}


struct IpcMessage_OpenProject : public BaseIpcMessage<IpcMessage_OpenProject> {
  static constexpr IpcId kIpcId = IpcId::OpenProject;
  std::string project_path;
};
template<typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_OpenProject& value) {
  Reflect(visitor, value.project_path);
}







struct IpcMessage_LanguageServerRequest : public BaseIpcMessage<IpcMessage_LanguageServerRequest> {
  static constexpr IpcId kIpcId = IpcId::LanguageServerRequest;
  // TODO: provide a way to get the request state.
  lsMethodId method_id;
};
template<typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_LanguageServerRequest& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(method_id);
  REFLECT_MEMBER_END();
}


struct IpcMessage_DocumentSymbolsRequest : public BaseIpcMessage<IpcMessage_DocumentSymbolsRequest> {
  static constexpr IpcId kIpcId = IpcId::DocumentSymbolsRequest;
  RequestId request_id;
  std::string document;
};
template<typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_DocumentSymbolsRequest& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(request_id);
  REFLECT_MEMBER(document);
  REFLECT_MEMBER_END();
}


struct IpcMessage_DocumentSymbolsResponse : public BaseIpcMessage<IpcMessage_DocumentSymbolsResponse> {
  static constexpr IpcId kIpcId = IpcId::DocumentSymbolsResponse;
  RequestId request_id;
  std::vector<lsSymbolInformation> symbols;
};
template<typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_DocumentSymbolsResponse& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(request_id);
  REFLECT_MEMBER(symbols);
  REFLECT_MEMBER_END();
}

struct IpcMessage_WorkspaceSymbolsRequest : public BaseIpcMessage<IpcMessage_WorkspaceSymbolsRequest> {
  static constexpr IpcId kIpcId = IpcId::WorkspaceSymbolsRequest;
  RequestId request_id;
  std::string query;
};
template<typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_WorkspaceSymbolsRequest& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(request_id);
  REFLECT_MEMBER(query);
  REFLECT_MEMBER_END();
}


struct IpcMessage_WorkspaceSymbolsResponse : public BaseIpcMessage<IpcMessage_WorkspaceSymbolsResponse> {
  static constexpr IpcId kIpcId = IpcId::WorkspaceSymbolsResponse;
  RequestId request_id;
  std::vector<lsSymbolInformation> symbols;
};
template<typename TVisitor>
void Reflect(TVisitor& visitor, IpcMessage_WorkspaceSymbolsResponse& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(request_id);
  REFLECT_MEMBER(symbols);
  REFLECT_MEMBER_END();
}

























void QueryDbMainLoop(IpcServer* ipc, QueryableDatabase* db) {
  std::vector<std::unique_ptr<IpcMessage>> messages = ipc->TakeMessages();

  for (auto& message : messages) {
    std::cerr << "Processing message " << static_cast<int>(message->ipc_id) << std::endl;

    switch (message->ipc_id) {
    case IpcId::Quit: {
      break;
    }

    case IpcId::IsAlive: {
      IpcMessage_IsAlive response;
      ipc->SendToClient(0, &response); // todo: make non-blocking
      break;
    }

    case IpcId::OpenProject: {
      IpcMessage_OpenProject* msg = static_cast<IpcMessage_OpenProject*>(message.get());
      std::string path = msg->project_path;


      std::vector<CompilationEntry> entries = LoadCompilationEntriesFromDirectory(path);
      for (int i = 0; i < entries.size(); ++i) {
        const CompilationEntry& entry = entries[i];
        std::string filepath = path + "/" + entry.filename;
        std::cerr << "[" << i << "/" << (entries.size() - 1) << "] Parsing file " << filepath << std::endl;
        IndexedFile file = Parse(filepath, entry.args);
        IndexUpdate update(file);
        db->ApplyIndexUpdate(&update);
      }
      std::cerr << "Done" << std::endl;
      break;
    }

    case IpcId::DocumentSymbolsRequest: {
      auto msg = static_cast<IpcMessage_DocumentSymbolsRequest*>(message.get());

      IpcMessage_DocumentSymbolsResponse response;
      response.request_id = msg->request_id;

      std::cerr << "Wanted file " << msg->document << std::endl;
      for (auto& file : db->files) {
        std::cerr << " - Have file " << file.file_id << std::endl;

        // TODO: make sure we normalize ids!
        // TODO: hashmap lookup.
        if (file.file_id == msg->document) {
          std::cerr << "Found file" << std::endl;


          for (UsrRef ref : file.outline) {
            SymbolIdx symbol = db->usr_to_symbol[ref.usr];

            lsSymbolInformation info;
            info.location.range.start.line = ref.loc.line - 1; // TODO: cleanup indexer to negate by 1.
            info.location.range.start.character = ref.loc.column - 1; // TODO: cleanup indexer to negate by 1.
                                                                      // TODO: store range information.
            info.location.range.end.line = info.location.range.start.line;
            info.location.range.end.character = info.location.range.start.character;

            // TODO: cleanup namespace/naming so there is only one SymbolKind.
            switch (symbol.kind) {
            case ::SymbolKind::Type:
            {
              QueryableTypeDef& def = db->types[symbol.idx];
              info.name = def.def.qualified_name;
              info.kind = lsSymbolKind::Class;
              break;
            }
            case SymbolKind::Func:
            {
              QueryableFuncDef& def = db->funcs[symbol.idx];
              info.name = def.def.qualified_name;
              if (def.def.declaring_type.has_value()) {
                info.kind = lsSymbolKind::Method;
                Usr declaring = def.def.declaring_type.value();
                info.containerName = db->types[db->usr_to_symbol[declaring].idx].def.qualified_name;
              }
              else {
                info.kind = lsSymbolKind::Function;
              }
              break;
            }
            case ::SymbolKind::Var:
            {
              QueryableVarDef& def = db->vars[symbol.idx];
              info.name = def.def.qualified_name;
              info.kind = lsSymbolKind::Variable;
              break;
            }
            };

            // TODO
            //info.containerName = "fooey";

            response.symbols.push_back(info);

          }
          break;
        }
      }




      ipc->SendToClient(0, &response);

      break;
    }

    case IpcId::WorkspaceSymbolsRequest: {
      auto msg = static_cast<IpcMessage_WorkspaceSymbolsRequest*>(message.get());

      IpcMessage_WorkspaceSymbolsResponse response;
      response.request_id = msg->request_id;

      std::cerr << "- Considering " << db->qualified_names.size() << " candidates " << std::endl;

      for (int i = 0; i < db->qualified_names.size(); ++i) {
        const std::string& name = db->qualified_names[i];
        //std::cerr << "- Considering " << name << std::endl;

        if (name.find(msg->query) != std::string::npos) {

          lsSymbolInformation info;
          info.name = name;

          SymbolIdx symbol = db->symbols[i];

          // TODO: dedup this code w/ above (ie, add ctor to convert symbol to SymbolInformation)
          switch (symbol.kind) {
            // TODO: file
          case ::SymbolKind::Type:
          {
            QueryableTypeDef& def = db->types[symbol.idx];
            info.name = def.def.qualified_name;
            info.kind = lsSymbolKind::Class;

            if (def.def.definition.has_value()) {
              info.location.uri.SetPath(def.def.definition->path);
              info.location.range.start.line = def.def.definition->line - 1;
              info.location.range.start.character = def.def.definition->column - 1;
            }
            break;
          }
          case ::SymbolKind::Func:
          {
            QueryableFuncDef& def = db->funcs[symbol.idx];
            info.name = def.def.qualified_name;
            if (def.def.declaring_type.has_value()) {
              info.kind = lsSymbolKind::Method;
              Usr declaring = def.def.declaring_type.value();
              info.containerName = db->types[db->usr_to_symbol[declaring].idx].def.qualified_name;
            }
            else {
              info.kind = lsSymbolKind::Function;
            }

            if (def.def.definition.has_value()) {
              info.location.uri.SetPath(def.def.definition->path);
              info.location.range.start.line = def.def.definition->line - 1;
              info.location.range.start.character = def.def.definition->column - 1;
            }
            break;
          }
          case ::SymbolKind::Var:
          {
            QueryableVarDef& def = db->vars[symbol.idx];
            info.name = def.def.qualified_name;
            info.kind = lsSymbolKind::Variable;

            if (def.def.definition.has_value()) {
              info.location.uri.SetPath(def.def.definition->path);
              info.location.range.start.line = def.def.definition->line - 1;
              info.location.range.start.character = def.def.definition->column - 1;
            }
            break;
          }
          };




          // TODO: store range information.
          info.location.range.end.line = info.location.range.start.line;
          info.location.range.end.character = info.location.range.start.character;

          response.symbols.push_back(info);

        }


      }


      ipc->SendToClient(0, &response);
      break;
    }

    default: {
      std::cerr << "Unhandled IPC message with kind " << static_cast<int>(message->ipc_id) << std::endl;
      exit(1);
    }
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
void LanguageServerStdinLoop(IpcClient* ipc) {
  while (true) {
    std::unique_ptr<InMessage> message = ParseMessage();

    // Message parsing can fail if we don't recognize the method.
    if (!message)
      continue;

    std::cerr << "[info]: Got message of type " << MethodIdToString(message->method_id) << std::endl;
    switch (message->method_id) {
    case lsMethodId::Initialize:
    {
      auto request = static_cast<In_InitializeRequest*>(message.get());
      if (request->params.rootUri) {
        std::string project_path = request->params.rootUri->GetPath();
        std::cerr << "Initialize in directory " << project_path << " with uri " << request->params.rootUri->raw_uri << std::endl;
        IpcMessage_OpenProject open_project;
        open_project.project_path = project_path;
        ipc->SendToServer(&open_project);
      }

      auto response = Out_InitializeResponse();
      response.id = message->id.value();
      response.result.capabilities.documentSymbolProvider = true;
      response.result.capabilities.workspaceSymbolProvider = true;
      response.Send();
      break;
    }

    case lsMethodId::TextDocumentDocumentSymbol:
    {
      // TODO: response should take id as input.
      // TODO: message should not have top-level id.
      auto request = static_cast<In_DocumentSymbolRequest*>(message.get());

      IpcMessage_DocumentSymbolsRequest ipc_request;
      ipc_request.request_id = request->id.value();
      ipc_request.document = request->params.textDocument.uri.GetPath();
      std::cerr << "Request textDocument=" << ipc_request.document << std::endl;
      ipc->SendToServer(&ipc_request);
      break;
    }

    case lsMethodId::WorkspaceSymbol:
    {
      auto request = static_cast<In_WorkspaceSymbolRequest*>(message.get());
      IpcMessage_WorkspaceSymbolsRequest ipc_request;
      ipc_request.request_id = request->id.value();
      ipc_request.query = request->params.query;
      std::cerr << "Request query=" << ipc_request.query << std::endl;
      ipc->SendToServer(&ipc_request);
      break;
    }
    }
  }
}

void LanguageServerMainLoop(IpcClient* ipc) {
  std::vector<std::unique_ptr<IpcMessage>> messages = ipc->TakeMessages();
  for (auto& message : messages) {
    switch (message->ipc_id) {
    case IpcId::Quit: {
      exit(0);
      break;
    }

    case IpcId::DocumentSymbolsResponse: {
      auto msg = static_cast<IpcMessage_DocumentSymbolsResponse*>(message.get());

      auto response = Out_DocumentSymbolResponse();
      response.id = msg->request_id;
      response.result = msg->symbols;
      response.Send();
      std::cerr << "Send symbol response to client (" << response.result.size() << " symbols)" << std::endl;
      break;
    }

    case IpcId::WorkspaceSymbolsResponse: {
      auto msg = static_cast<IpcMessage_WorkspaceSymbolsResponse*>(message.get());

      auto response = Out_WorkspaceSymbolResponse();
      response.id = msg->request_id;
      response.result = msg->symbols;
      response.Send();
      std::cerr << "Send symbol response to client (" << response.result.size() << " symbols)" << std::endl;
      break;
    }

    default: {
      std::cerr << "Unhandled IPC message with kind " << static_cast<int>(message->ipc_id) << std::endl;
      exit(1);
    }
    }
  }
}

void LanguageServerMain(std::string process_name) {
  IpcClient client_ipc("languageserver", 0);

  // Discard any left-over messages from previous runs.
  client_ipc.TakeMessages();

  // Emit an alive check. Sleep so the server has time to respond.
  IpcMessage_IsAlive check_alive;
  client_ipc.SendToServer(&check_alive);

  // TODO: Tune this value or make it configurable.
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  // Check if we got an IsAlive message back.
  std::vector<std::unique_ptr<IpcMessage>> messages = client_ipc.TakeMessages();
  bool has_server = false;
  for (auto& message : messages) {
    if (IpcId::IsAlive == message->ipc_id) {
      has_server = true;
      break;
    }
  }

  // No server is running. Start it.
#if false
  if (!has_server) {
    if (process_name.empty())
      return;

    Process p(process_name + " --querydb", "",
      /*stdout*/[](const char* bytes, size_t n) {
      for (int i = 0; i < n; ++i)
        std::cerr << bytes[i];
    },
      /*stderr*/[](const char* bytes, size_t n) {
      for (int i = 0; i < n; ++i)
        std::cerr << bytes[i];
    },
      /*open_stdin*/false);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // Pass empty process name so we only try to start the querydb once.
    LanguageServerMain("");
    return;
  }
#endif

  // for debugging attach
  //std::this_thread::sleep_for(std::chrono::seconds(4));


  std::thread stdio_reader(&LanguageServerStdinLoop, &client_ipc);


  // No server. Run it in-process.
  if (!has_server) {

    QueryableDatabase db;
    IpcServer server_ipc("languageserver");

    while (true) {
      QueryDbMainLoop(&server_ipc, &db);
      LanguageServerMainLoop(&client_ipc);
      // TODO: use a condition variable.
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  }

  else {
    while (true) {
      LanguageServerMainLoop(&client_ipc);
      // TODO: use a condition variable.
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  }
}















































int main(int argc, char** argv) {
  if (argc == 1) {
    RunTests();
    return 0;
  }

  // We need to write to stdout in binary mode because in Windows, writing
  // \n will implicitly write \r\n. Language server API will ignore a
  // \r\r\n split request.
#ifdef _WIN32
  _setmode(_fileno(stdout), O_BINARY);
  _setmode(_fileno(stdin), O_BINARY);
#endif

  IpcRegistry::instance()->Register<IpcMessage_Quit>(IpcId::Quit);
  IpcRegistry::instance()->Register<IpcMessage_IsAlive>(IpcId::IsAlive);
  IpcRegistry::instance()->Register<IpcMessage_OpenProject>(IpcId::OpenProject);

  IpcRegistry::instance()->Register<IpcMessage_DocumentSymbolsRequest>(IpcId::DocumentSymbolsRequest);
  IpcRegistry::instance()->Register<IpcMessage_DocumentSymbolsResponse>(IpcId::DocumentSymbolsResponse);
  IpcRegistry::instance()->Register<IpcMessage_WorkspaceSymbolsRequest>(IpcId::WorkspaceSymbolsRequest);
  IpcRegistry::instance()->Register<IpcMessage_WorkspaceSymbolsResponse>(IpcId::WorkspaceSymbolsResponse);

  MessageRegistry::instance()->Register<In_CancelRequest>();
  MessageRegistry::instance()->Register<In_InitializeRequest>();
  MessageRegistry::instance()->Register<In_InitializedNotification>();
  MessageRegistry::instance()->Register<In_DocumentSymbolRequest>();
  MessageRegistry::instance()->Register<In_WorkspaceSymbolRequest>();






  std::unordered_map<std::string, std::string> options = ParseOptions(argc, argv);

  if (HasOption(options, "--language-server")) {
    std::cerr << "Running language server" << std::endl;
    LanguageServerMain(argv[0]);
    return 0;
  }
  else if (HasOption(options, "--querydb")) {
    std::cerr << "Running querydb" << std::endl;
    QueryableDatabase db;
    IpcServer ipc("languageserver");
    while (true) {
      QueryDbMainLoop(&ipc, &db);
      // TODO: use a condition variable.
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return 0;
  }
  else {
    std::cerr << "Running language server" << std::endl;
    LanguageServerMain(argv[0]);
    return 0;
  }

  return 1;
}
