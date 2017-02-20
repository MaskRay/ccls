#include <cstdint>
#include <optional>
#include <unordered_map>

using FileId = uint64_t;

struct FileDatabase {
  std::unordered_map<std::string, FileId> filename_to_file_id;
  std::unordered_map<FileId, std::string> file_id_to_filename;
};

struct SymbolIdx {
  std::optional<uint64_t> type_idx;
  std::optional<uint64_t> func_idx;
  std::optional<uint64_t> var_idx;
};

struct File {
  // Symbols declared in the file.
  std::vector<SymbolIdx> declared_symbols;
  // Symbols which have definitions in the file.
  std::vector<SymbolIdx> defined_symbols;
};

struct TypeDef {};
struct FuncDef {};
struct VarDef {};

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

struct TaskManager {

};
