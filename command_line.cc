#include <iostream>
#include <string>
#include <unordered_map>

#include "compilation_database_loader.h"
#include "indexer.h"
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

#include "ipc.h"

void IndexerServerMain() {
  IpcServer ipc("language_server");

  while (true) {
    std::vector<std::unique_ptr<BaseIpcMessage>> messages = ipc.TakeMessages();

    std::cout << "Server has " << messages.size() << " messages" << std::endl;
    for (auto& message : messages) {
      switch (message->kind) {
      case JsonMessage::Kind::IsAlive:
      {
        IpcMessage_IsAlive response;
        ipc.SendToClient(0, &response); // todo: make non-blocking
        break;
      }
      default:
        std::cerr << "Unhandled IPC message with kind " << static_cast<int>(message->kind) << std::endl;
        exit(1);
        break;
      }
    }

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(20ms);
  }
}

void LanguageServerMain() {
  IpcClient ipc("language_server", 0);

  // Discard any left-over messages from previous runs.
  ipc.TakeMessages();

  // Emit an alive check. Sleep so the server has time to respond.
  IpcMessage_IsAlive check_alive;
  ipc.SendToServer(&check_alive);
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(50ms); // TODO: Tune this value or make it configurable.

  // Check if we got an IsAlive message back.
  std::vector<std::unique_ptr<BaseIpcMessage>> messages = ipc.TakeMessages();
  bool has_server = false;
  for (auto& message : messages) {
    if (message->kind == JsonMessage::Kind::IsAlive) {
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
}

int main(int argc, char** argv) {
  if (argc == 1)
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
