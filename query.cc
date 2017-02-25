#include "query.h"

#include <cstdint>
#include <unordered_map>
#include <string>
#include <iostream>

#include "compilation_database_loader.h"
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



// There are two sources of reindex updates: the (single) definition of a
// symbol has changed, or one of many users of the symbol has changed.
//
// For simplicitly, if the single definition has changed, we update all of the
// associated single-owner definition data. See |Update*DefId|.
//
// If one of the many symbol users submits an update, we store the update such
// that it can be merged with other updates before actually being applied to
// the main database. See |MergeableUpdate|.

template<typename TId, typename TValue>
struct MergeableUpdate {
  // The type/func/var which is getting new usages.
  TId id;
  // Entries to add and remove.
  std::vector<TValue> to_add;
  std::vector<TValue> to_remove;
};

struct QueryableTypeDef {
  TypeDefDefinitionData def;
  std::vector<TypeId> derived;
  std::vector<Location> uses;

  using DefUpdate = TypeDefDefinitionData;
  using DerivedUpdate = MergeableUpdate<TypeId, TypeId>;
  using UsesUpdate = MergeableUpdate<TypeId, Location>;
};

struct QueryableFuncDef {
  FuncDefDefinitionData def;
  std::vector<Location> declarations;
  std::vector<FuncId> derived;
  std::vector<FuncRef> callers;
  std::vector<Location> uses;

  using DefUpdate = FuncDefDefinitionData;
  using DeclarationsUpdate = MergeableUpdate<FuncId, Location>;
  using DerivedUpdate = MergeableUpdate<FuncId, FuncId>;
  using CallersUpdate = MergeableUpdate<FuncId, FuncRef>;
  using UsesUpdate = MergeableUpdate<FuncId, Location>;
};

struct QueryableVarDef {
  VarDefDefinitionData def;
  std::vector<Location> uses;

  using DefUpdate = VarDefDefinitionData;
  using UsesUpdate = MergeableUpdate<VarId, Location>;
};

struct QueryableFile {
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
struct QueryableDatabase {
  // Indicies between lookup vectors are related to symbols, ie, index 5 in
  // |qualified_names| matches index 5 in |symbols|.
  std::vector<QueryableEntry> qualified_names;
  std::vector<SymbolIdx> symbols;

  // Raw data storage.
  std::vector<QueryableTypeDef> types;
  std::vector<QueryableFuncDef> funcs;
  std::vector<QueryableVarDef> vars;

  // |files| is indexed by FileId. Retrieve a FileId from a path using
  // |file_locator|.
  FileDatabase file_locator;
  std::vector<QueryableFile> files;
};



struct Query {

};


struct CachedIndexedFile {
  // Path to the file indexed.
  std::string path;
  
  // Full in-memory storage for the index. Empty if not loaded into memory.
  // |path| can be used to fetch the index from disk.
  optional<rapidjson::Document> index;
};

struct DocumentDiff {

};
// Compute a diff between |original| and |updated|.
//rapidjson::Document DiffIndex(rapidjson::Document original, rapidjson::Document updated) {

//}




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

int main2(int argc, char** argv) {
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


    std::vector<IndexedFile> dbs;
    for (const CompilationEntry& entry : entries) {
      std::cout << "Parsing " << entry.filename << std::endl;
      IndexedFile db = Parse(entry.filename, entry.args);

      dbs.emplace_back(db);
      std::cout << db.ToString() << std::endl << std::endl;
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
