#include <iostream>
#include <string>
#include <unordered_map>

#include "compilation_database_loader.h"
#include "indexer.h"
#include "ipc.h"
#include "query.h"

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


struct IpcMessage_IsAlive : public BaseIpcMessage {
  static IpcMessageId id;
};

IpcMessageId IpcMessage_IsAlive::id = "IsAlive";








struct IpcMessage_DocumentSymbolsRequest : public BaseIpcMessage {
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


// Keep all types in the language_server_api namespace in sync with language server spec.
namespace language_server_api {
  using DocumentUri = std::string; // TODO

  struct Position {
    // Note: these are 0-based.
    int line;
    int character;
  };

  struct Range {
    Position start;
    Position end;
  };

  struct Location {
    DocumentUri uri;
    Range range;
  };

  enum class SymbolKind : int {
    File = 1,
    Module = 2,
    Namespace = 3,
    Package = 4,
    Class = 5,
    Method = 6,
    Property = 7,
    Field = 8,
    Constructor = 9,
    Enum = 10,
    Interface = 11,
    Function = 12,
    Variable = 13,
    Constant = 14,
    String = 15,
    Number = 16,
    Boolean = 17,
    Array = 18
  };

  struct SymbolInfo {
    std::string name;
    SymbolKind kind;
    Location location;
    std::string containerName;
  };
}

struct IpcMessage_DocumentSymbolsResponse : public BaseIpcMessage {
  std::vector<language_server_api::SymbolInfo> symbols;

  // BaseIpcMessage:
  static IpcMessageId id;
};
IpcMessageId IpcMessage_DocumentSymbolsResponse::id = "IpcMessage_DocumentSymbolsResponse";






void IndexerServerMain() {
  IpcServer ipc("languageserver");

  while (true) {
    std::vector<std::unique_ptr<BaseIpcMessage>> messages = ipc.TakeMessages();

    std::cout << "Server has " << messages.size() << " messages" << std::endl;
    for (auto& message : messages) {
      if (message->runtime_id == IpcMessage_IsAlive::id) {
        IpcMessage_IsAlive response;
        ipc.SendToClient(0, &response); // todo: make non-blocking
        break;
      }
      else {
        std::cerr << "Unhandled IPC message with kind " << message->runtime_id << " (hash " << message->hashed_runtime_id << ")" << std::endl;
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

// Main loop for the language server. |ipc| is connected to
// a server.
void LanguageServerLoop(IpcClient& ipc) {
  while (true) {
    std::string input;
    std::cin >> input;

    std::cout << "got input " << input << std::endl << std::endl;

    if (input == "references") {

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
  std::vector<std::unique_ptr<BaseIpcMessage>> messages = ipc.TakeMessages();
  bool has_server = false;
  for (auto& message : messages) {
    if (message->runtime_id == IpcMessage_IsAlive::id) {
      has_server = true;
      break;
    }
  }

  // No server is running. Start it.
  if (!has_server) {
    std::cerr << "Unable to detect running indexer server" << std::endl;
    exit(1);
  }

  std::cout << "Found indexer server" << std::endl;
  LanguageServerLoop(ipc);
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
  IpcRegistry::instance()->Register<IpcMessage_IsAlive>();
  IpcRegistry::instance()->Register<IpcMessage_DocumentSymbolsRequest>();
  IpcRegistry::instance()->Register<IpcMessage_DocumentSymbolsResponse>();


  if (argc == 2)
    LanguageServerMain();
  else
    IndexerServerMain();
  return 0;

  std::unordered_map<std::string, std::string> options = ParseOptions(argc, argv);

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
