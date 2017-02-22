#include <cstdint>
#include <unordered_map>
#include <string>
#include <iostream>

#include "compilation_database_loader.h"

//#include "cxxopts.hpp"
#include "optional.h"
#include "indexer.h"

struct FileDatabase {
  std::unordered_map<std::string, FileId> filename_to_file_id;
  std::unordered_map<FileId, std::string> file_id_to_filename;
};

enum class SymbolKind { Type, Func, Var };
struct SymbolIdx {
  SymbolKind kind;
  union {
    uint64_t type_idx;
    uint64_t func_idx;
    uint64_t var_idx;
  };
};

struct File {
  // Symbols declared in the file.
  std::vector<SymbolIdx> declared_symbols;
  // Symbols which have definitions in the file.
  std::vector<SymbolIdx> defined_symbols;
};

struct QueryableEntry {
  const char* const str;
};

// The query database is heavily optimized for fast queries. It is stored
// in-memory.
struct QueryDatabase {
  // Indicies between lookup vectors are related to symbols, ie, index 5 in
  // |qualified_names| matches index 5 in |symbols|.
  std::vector<QueryableEntry> qualified_names;
  std::vector<SymbolIdx> symbols;

  // Raw data storage.
  std::vector<TypeDef> types;
  std::vector<FuncDef> funcs;
  std::vector<VarDef> vars;

  // |files| is indexed by FileId. Retrieve a FileId from a path using
  // |file_locator|.
  FileDatabase file_locator;
  std::vector<File> files;
};




// Task running in a separate process, parsing a file into something we can
// import.
struct ParseTask {};
// Completed parse task that wants to import content into the global database.
// Runs in main process, primary thread. Stops all other threads.
struct IndexImportTask {};
// Completed parse task that wants to update content previously imported into
// the global database. Runs in main process, primary thread. Stops all other
// threads.
//
// Note that this task just contains a set of operations to apply to the global
// database. The operations come from a diff based on the previously indexed
// state in comparison to the newly indexed state.
//
// TODO: We may be able to run multiple freshen and import tasks in parallel if
//       we restrict what ranges of the db they may change.
struct IndexFreshenTask {};
// Task running a query against the global database. Run in main process,
// separate thread.
struct QueryTask {};


// NOTE: When something enters a value into master db, it will have to have a
//       ref count, since multiple parsings could enter it (unless we require
//       that it be defined in that declaration unit!)
struct TaskManager {

};


struct Query {

};

void fail(const std::string& message) {
  std::cerr << "Fatal error: " << message << std::endl;
  std::exit(1);
}

enum class PreferredSymbolLocation {
  Declaration,
  Definition
};
std::istream& operator >> (std::istream& is, PreferredSymbolLocation& obj) {
  std::string content;
  is >> content;

  if (content == "declaration")
    obj = PreferredSymbolLocation::Declaration;
  else if (content == "definition")
    obj = PreferredSymbolLocation::Definition;
  else
    is.setstate(std::ios::failbit);

  return is;
}

// NOTE: If updating this enum, make sure to also update the parser and the
//       help text.
enum class Command {
  Callees,
  Callers,
  FindAllUsages,
  FindInterestingUsages,
  GotoReferenced,
  Hierarchy,
  Outline,
  Search
};

//std::ostream& operator<<(std::ostream& os, const Command& obj) {
  // write obj to stream
//  return os;
//}
std::istream& operator >> (std::istream& is, Command& obj) {
  std::string content;
  is >> content;

  if (content == "callees")
    obj = Command::Callees;
  else if (content == "callers")
    obj = Command::Callers;
  else if (content == "find-all-usages")
    obj = Command::FindAllUsages;
  else if (content == "find-interesting-usages")
    obj = Command::FindInterestingUsages;
  else if (content == "goto-referenced")
    obj = Command::GotoReferenced;
  else if (content == "hierarchy")
    obj = Command::Hierarchy;
  else if (content == "outline")
    obj = Command::Outline;
  else if (content == "search")
    obj = Command::Search;
  else
    is.setstate(std::ios::failbit);

  return is;
}

// TODO: I think we can run libclang multiple times in one process. So we might
//       only need two processes. Still, for perf reasons it would be good if
//       we could stay in one process.
// TODO: allow user to store configuration as json? file in home dir; also
//       allow local overrides (scan up dirs)
// TODO: add opt to dump config when starting (--dump-config)
// TODO: allow user to decide some indexer choices, ie, do we define
// TODO: may want to run indexer in separate process to avoid indexer/compiler crashes?

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

int main(int argc, char** argv) {
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


    std::vector<ParsingDatabase> dbs;
    for (const CompilationEntry& entry : entries) {
      std::cout << "Parsing " << entry.filename << std::endl;
      ParsingDatabase db = Parse(entry.filename, entry.args);

      dbs.emplace_back(db);
      std::cout << db.ToString() << std::endl << std::endl;
    }

    std::cin.get();
    exit(0);
  }


  std::cout << "Invalid arguments. Try --help.";
  exit(1);
  return 0;
}
