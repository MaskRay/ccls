#include <iostream>
#include <string>
#include <unordered_map>
#include <thread>

#include "compilation_database_loader.h"
#include "indexer.h"
#include "ipc.h"
#include "query.h"
#include "language_server_api.h"

#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

bool ParsePreferredSymbolLocation(const std::string& content, PreferredSymbolLocation* obj) {
#define PARSE_AS(name, string)      \
  if (content == #string) {         \
    *obj = name;                    \
    return true;                    \
  }

  PARSE_AS(PreferredSymbolLocation::Declaration, "declaration");
  PARSE_AS(PreferredSymbolLocation::Definition, "definition");

  return false;
#undef PARSE_AS
}

bool ParseCommand(const std::string& content, Command* obj) {
#define PARSE_AS(name, string)      \
  if (content == #string) {         \
    *obj = name;                    \
    return true;                    \
  }

  PARSE_AS(Command::Callees, "callees");
  PARSE_AS(Command::Callers, "callers");
  PARSE_AS(Command::FindAllUsages, "find-all-usages");
  PARSE_AS(Command::FindInterestingUsages, "find-interesting-usages");
  PARSE_AS(Command::GotoReferenced, "goto-referenced");
  PARSE_AS(Command::Hierarchy, "hierarchy");
  PARSE_AS(Command::Outline, "outline");
  PARSE_AS(Command::Search, "search");

  return false;
#undef PARSE_AS
}


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

/*

// Connects to a running --project-directory instance. Forks
// and creates it if not running.
//
// Implements language server spec.
indexer.exe --language-server

// Holds the runtime db that the --language-server instance
// runs queries against.
indexer.exe --project-directory /work2/chrome/src

// Created from the --project-directory (server) instance
indexer.exe --index-file /work2/chrome/src/chrome/foo.cc

// Configuration data is read from a JSON file.
{
  "max_threads": 40,
  "cache_directory": "/work/indexer_cache/"

}
*/


struct IpcMessage_IsAlive : public BaseIpcMessage<IpcMessage_IsAlive> {
  static IpcMessageId id;
};

IpcMessageId IpcMessage_IsAlive::id = "IsAlive";













struct IpcMessage_DocumentSymbolsRequest : public BaseIpcMessage<IpcMessage_DocumentSymbolsRequest> {
  std::string document;

  // BaseIpcMessage:
  static IpcMessageId id;
  void Serialize(Writer& writer) override {
    writer.String(document.c_str());
  }
  void Deserialize(Reader& reader) override {
    document = reader.GetString();
  }
};
IpcMessageId IpcMessage_DocumentSymbolsRequest::id = "IpcMessage_DocumentSymbolsRequest";

struct IpcMessage_DocumentSymbolsResponse : public BaseIpcMessage<IpcMessage_DocumentSymbolsResponse> {
  std::vector<language_server_api::SymbolInformation> symbols;

  // BaseIpcMessage:
  static IpcMessageId id;
};
IpcMessageId IpcMessage_DocumentSymbolsResponse::id = "IpcMessage_DocumentSymbolsResponse";





void QueryDbMain() {
  IpcServer ipc("languageserver");

  while (true) {
    std::vector<std::unique_ptr<BaseIpcMessageElided>> messages = ipc.TakeMessages();

    for (auto& message : messages) {
      std::cout << "Processing message " << message->runtime_id() << " (hash " << message->hashed_runtime_id() << ")" << std::endl;

      if (message->runtime_id() == IpcMessage_IsAlive::id) {
        IpcMessage_IsAlive response;
        ipc.SendToClient(0, &response); // todo: make non-blocking
        break;
      }
      else {
        std::cerr << "Unhandled IPC message with kind " << message->runtime_id() << " (hash " << message->hashed_runtime_id() << ")" << std::endl;
        exit(1);
        break;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}

void EmitReferences(IpcClient& ipc) {

}

// Separate thread whose only job is to read from stdin and
// dispatch read commands to the actual indexer program. This
// cannot be done on the main thread because reading from std::cin
// blocks.
void LanguageServerStdinToServerDispatcher(IpcClient& ipc) {
  while (true) {
    std::string input;
    std::cin >> input;

  }
}


void ParseRpc(const std::string& method, const rapidjson::GenericValue<rapidjson::UTF8<>>& params) {
}

std::unique_ptr<language_server_api::InMessage> ParseMessage() {
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

  assert(content_length >= 0);

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

  return language_server_api::MessageRegistry::instance()->Parse(document);

  /*
  std::string id;
  if (document["id"].IsString())
  id = document["id"].GetString();
  else
  id = std::to_string(document["id"].GetInt());
  std::string method = document["method"].GetString();
  auto& params = document["params"];


  // Send initialize response.
  {
    std::string content =
      R"foo({
      "jsonrpc": "2.0",
      "id": 0,
      "result": {
        "capabilities": {
          "documentSymbolProvider": true
        }
      }
    })foo";
    std::cout << "Content-Length: " << content.size();
    std::cout << (char)13 << char(10) << char(13) << char(10);
    std::cout << content;
  }
  */
}



// Main loop for the language server. |ipc| is connected to
// a server.
void LanguageServerLoop(IpcClient* ipc) {
  using namespace language_server_api;

  while (true) {
    std::unique_ptr<InMessage> message = ParseMessage();

    // Message parsing can fail if we don't recognize the method.
    if (!message)
      continue;

    std::cerr << "[info]: Got message of type " << MethodIdToString(message->method_id) << std::endl;
    switch (message->method_id) {
    case MethodId::Initialize:
    {
      // TODO: response should take id as input.
      // TODO: message should not have top-level id.
      auto response = Out_InitializeResponse();
      response.id = message->id.value();
      response.result.capabilities.documentSymbolProvider = true;
      response.Send();
      break;
    }

    case MethodId::TextDocumentDocumentSymbol:
    {
      auto response = Out_DocumentSymbolResponse();
      response.id = message->id.value();

      for (int i = 0; i < 2500; ++i) {
        SymbolInformation info;
        info.containerName = "fooContainer";
        info.kind = language_server_api::SymbolKind::Field;
        info.location.range.start.line = 5;
        info.location.range.end.character = 20;
        info.location.range.end.line = 5;
        info.location.range.end.character = 25;
        info.name = "Foobar";
        response.result.push_back(info);
      }

      response.Send();
      break;
    }
    }
  }
}




void LanguageServerMain() {
  IpcClient ipc("languageserver", 0);

  // Discard any left-over messages from previous runs.
  ipc.TakeMessages();

  // Emit an alive check. Sleep so the server has time to respond.
  IpcMessage_IsAlive check_alive;
  ipc.SendToServer(&check_alive);

  // TODO: Tune this value or make it configurable.
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  // Check if we got an IsAlive message back.
  std::vector<std::unique_ptr<BaseIpcMessageElided>> messages = ipc.TakeMessages();
  bool has_server = false;
  for (auto& message : messages) {
    if (message->runtime_id() == IpcMessage_IsAlive::id) {
      has_server = true;
      break;
    }
  }

  // No server is running. Start it.
  //if (!has_server) {
  //  std::cerr << "Unable to detect running indexer server" << std::endl;
  //  exit(1);
  //}

  std::thread stdio_reader(&LanguageServerLoop, &ipc);

  //std::cout << "Found indexer server" << std::endl;
  //LanguageServerLoop(ipc);

  // TODO: This is used for debugging, so we can attach to the client.


  //std::cout << "garbagelkadklasldk" << std::endl;

  bool should_break = true;
  while (should_break)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  //std::this_thread::sleep_for(std::chrono::seconds(4));


  //std::cout.flush();
  /*
  language_server_api::ShowMessageOutNotification show;
  show.type = language_server_api::MessageType::Info;
  show.message = "hello";
  show.Send();
  */
}





#if false

struct IpcMessage_IsAlive : public BaseIpcMessage {
  IpcMessage_IsAlive();

  // BaseIpcMessage:
  void Serialize(Writer& writer) override;
  void Deserialize(Reader& reader) override;
};

struct IpcMessage_ImportIndex : public BaseIpcMessage {
  std::string path;

  IpcMessage_ImportIndex();

  // BaseMessage:
  void Serialize(Writer& writer) override;
  void Deserialize(Reader& reader) override;
};

struct IpcMessage_CreateIndex : public BaseIpcMessage {
  std::string path;
  std::vector<std::string> args;

  IpcMessage_CreateIndex();

  // BaseMessage:
  void Serialize(Writer& writer) override;
  void Deserialize(Reader& reader) override;
};


IpcMessage_IsAlive::IpcMessage_IsAlive() {
  kind = JsonMessage::Kind::IsAlive;
}

void IpcMessage_IsAlive::Serialize(Writer& writer) {}

void IpcMessage_IsAlive::Deserialize(Reader& reader) {}

IpcMessage_ImportIndex::IpcMessage_ImportIndex() {
  kind = JsonMessage::Kind::ImportIndex;
}

void IpcMessage_ImportIndex::Serialize(Writer& writer) {
  writer.StartObject();
  ::Serialize(writer, "path", path);
  writer.EndObject();
}
void IpcMessage_ImportIndex::Deserialize(Reader& reader) {
  ::Deserialize(reader, "path", path);
}

IpcMessage_CreateIndex::IpcMessage_CreateIndex() {
  kind = JsonMessage::Kind::CreateIndex;
}

void IpcMessage_CreateIndex::Serialize(Writer& writer) {
  writer.StartObject();
  ::Serialize(writer, "path", path);
  ::Serialize(writer, "args", args);
  writer.EndObject();
}
void IpcMessage_CreateIndex::Deserialize(Reader& reader) {
  ::Deserialize(reader, "path", path);
  ::Deserialize(reader, "args", args);
}
#endif

int main(int argc, char** argv) {
  // We need to write to stdout in binary mode because in Windows, writing
  // \n will implicitly write \r\n. Language server API will ignore a
  // \r\r\n split request.
#ifdef _WIN32
  _setmode(_fileno(stdout), O_BINARY);
  _setmode(_fileno(stdin), O_BINARY);
#endif

  std::cerr << "Starting language server" << std::endl;

  IpcRegistry::instance()->Register<IpcMessage_IsAlive>();
  IpcRegistry::instance()->Register<IpcMessage_DocumentSymbolsRequest>();
  IpcRegistry::instance()->Register<IpcMessage_DocumentSymbolsResponse>();

  language_server_api::MessageRegistry::instance()->Register<language_server_api::In_CancelRequest>();
  language_server_api::MessageRegistry::instance()->Register<language_server_api::In_InitializeRequest>();
  language_server_api::MessageRegistry::instance()->Register<language_server_api::In_InitializedNotification>();
  language_server_api::MessageRegistry::instance()->Register<language_server_api::In_DocumentSymbolRequest>();

  std::unordered_map<std::string, std::string> options = ParseOptions(argc, argv);

  if (HasOption(options, "--language-server")) {
    LanguageServerMain();
    return 0;
  }
  if (HasOption(options, "--querydb")) {
    QueryDbMain();
    return 0;
  }


  LanguageServerMain();
  return 0;

  if (argc == 1 || options.find("--help") != options.end()) {
    std::cout << R"help(clang-indexer help:

  General:
    --help        Print this help information.
    --help-commands
                  Print all available query commands.
    --project     Path to compile_commands.json. Needed for the server, and
                  optionally by clients if there are multiple servers running.
    --print-config
                  Emit all configuration data this executable is using.


  Server:
    --server      If present, this binary will run in server mode. The binary
                  will not return until killed or an exit is requested. The
                  server computes and caches an index of the entire program
                  which is then queried by short-lived client processes. A
                  client is created by running this binary with a --command
                  flag.
    --cache-dir   Directory to cache the index and other useful information. If
                  a previous cache is present, the database will try to reuse
                  it. If this flag is not present, the database will be
                  in-memory only.
    --threads     Number of threads to use for indexing and querying tasks.
                  This value is optional; a good estimate is computed by
                  default.


  Client:
    --command     Execute a query command against the index. See
                  --command-help for a listing of valid commands and a
                  description of what they do. Presence of this flag indicates
                  that the indexer is in client mode; this flag is mutually
                  exclusive with --server.
    --location    Location of the query. Some commands require only a file,
                  other require a line and column as well. Format is
                  filename[:line:column]. For example, "foobar.cc" and
                  "foobar.cc:1:10" are valid inputs.
    --preferred-symbol-location
                  When looking up symbols, try to return either the
                  'declaration' or the 'definition'. Defaults to 'definition'.
)help";
    exit(0);
  }

  if (HasOption(options, "--help-commands")) {
    std::cout << R"(Available commands:

  callees:
  callers:
    Emit all functions (with location) that this function calls ("callees") or
    that call this function ("callers"). Requires a location.

  find-all-usages:
    Emit every usage of the given symbol. This is intended to support a rename
    refactoring. This output contains many uninteresting usages of symbols;
    prefer find-interesting-usges. Requires a location.

  find-interesting-usages:
    Emit only usages of the given symbol which are semantically interesting.
    Requires a location.

  goto-referenced:
    Find an associated reference (either definition or declaration) for the
    given symbol. Requires a location.

  hierarchy:
    List the type hierarchy (ie, inherited and derived members) for the given
    method or type. Requires a location.

  outline:
    Emit a file outline, listing all of the symbols in the file.

  search:
    Search for a symbol by name.
)";
    exit(0);
  }

  if (HasOption(options, "--project")) {
    std::vector<CompilationEntry> entries = LoadCompilationEntriesFromDirectory(options["--project"]);


    for (const CompilationEntry& entry : entries) {
      std::cout << "Parsing " << entry.filename << std::endl;
      QueryableDatabase db;
      IndexedFile file = Parse(entry.filename, entry.args);

      IndexUpdate update(file);
      db.ApplyIndexUpdate(&update);
      //std::cout << db.ToString() << std::endl << std::endl;
    }

    std::cin.get();
    exit(0);
  }

  if (HasOption(options, "--command")) {
    Command command;
    if (!ParseCommand(options["--command"], &command))
      Fail("Unknown command \"" + options["--command"] + "\"; see --help-commands");


  }

  std::cout << "Invalid arguments. Try --help.";
  exit(1);
  return 0;
}
