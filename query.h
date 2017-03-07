#pragma once

#include "indexer.h"
#include "serializer.h"

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

// NOTE: If updating this enum, make sure to also update the parser and the
//       help text.
enum class PreferredSymbolLocation {
  Declaration,
  Definition
};

using Usr = std::string;

// TODO: Switch over to QueryableLocation. Figure out if there is
//       a good way to get the indexer using it. I don't think so
//       since we may discover more files while indexing a file.
//
//       We could also reuse planned USR caching system for file
//       paths.
struct QueryableLocation {
  Usr path;
  int line;
  int column;
  bool interesting;

  QueryableLocation()
    : path(""), line(-1), column(-1), interesting(false) {}
  QueryableLocation(Usr path, int line, int column, bool interesting)
    : path(path), line(line), column(column), interesting(interesting) {}

  bool operator==(const QueryableLocation& other) const {
    // Note: We ignore |is_interesting|.
    return
      path == other.path &&
      line == other.line &&
      column == other.column;
  }
  bool operator!=(const QueryableLocation& other) const { return !(*this == other); }
  bool operator<(const QueryableLocation& o) const {
    return
      path < o.path &&
      line < o.line &&
      column < o.column &&
      interesting < o.interesting;
  }
};


struct UsrRef {
  Usr usr;
  QueryableLocation loc;

  UsrRef() {}
  UsrRef(Usr usr, QueryableLocation loc) : usr(usr), loc(loc) {}

  bool operator==(const UsrRef& other) const {
    return usr == other.usr && loc == other.loc;
  }
  bool operator!=(const UsrRef& other) const { return !(*this == other); }
  bool operator<(const UsrRef& other) const {
    return usr < other.usr && loc < other.loc;
  }
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

template<typename TValue>
struct MergeableUpdate {
  // The type/func/var which is getting new usages.
  Usr usr;
  // Entries to add and remove.
  std::vector<TValue> to_add;
  std::vector<TValue> to_remove;

  MergeableUpdate(Usr usr, const std::vector<TValue>& to_add, const std::vector<TValue>& to_remove)
    : usr(usr), to_add(to_add), to_remove(to_remove) {}
};

struct QueryableFile {
  using OutlineUpdate = MergeableUpdate<UsrRef>;

  Usr file_id;
  // Outline of the file (ie, all symbols).
  std::vector<UsrRef> outline;

  QueryableFile(const IndexedFile& indexed);
};

struct QueryableTypeDef {
  using DefUpdate = TypeDefDefinitionData<Usr, Usr, Usr, QueryableLocation>;
  using DerivedUpdate = MergeableUpdate<Usr>;
  using UsesUpdate = MergeableUpdate<QueryableLocation>;

  DefUpdate def;
  std::vector<Usr> derived;
  std::vector<QueryableLocation> uses;

  QueryableTypeDef(IdCache& id_cache, const IndexedTypeDef& indexed);
};

struct QueryableFuncDef {
  using DefUpdate = FuncDefDefinitionData<Usr, Usr, Usr, UsrRef, QueryableLocation>;
  using DeclarationsUpdate = MergeableUpdate<QueryableLocation>;
  using DerivedUpdate = MergeableUpdate<Usr>;
  using CallersUpdate = MergeableUpdate<UsrRef>;
  using UsesUpdate = MergeableUpdate<QueryableLocation>;

  DefUpdate def;
  std::vector<QueryableLocation> declarations;
  std::vector<Usr> derived;
  std::vector<UsrRef> callers;
  std::vector<QueryableLocation> uses;

  QueryableFuncDef(IdCache& id_cache, const IndexedFuncDef& indexed);
};

struct QueryableVarDef {
  using DefUpdate = VarDefDefinitionData<Usr, Usr, Usr, QueryableLocation>;
  using UsesUpdate = MergeableUpdate<QueryableLocation>;

  DefUpdate def;
  std::vector<QueryableLocation> uses;

  QueryableVarDef(IdCache& id_cache, const IndexedVarDef& indexed);
};

enum class SymbolKind { Invalid, File, Type, Func, Var };
struct SymbolIdx {
  SymbolKind kind;
  uint64_t idx;

  SymbolIdx() : kind(SymbolKind::Invalid), idx(-1) {} // Default ctor needed by stdlib. Do not use.
  SymbolIdx(SymbolKind kind, uint64_t idx) : kind(kind), idx(idx) {}
};


// TODO: We need to control Usr, std::vector allocation to make sure it happens on shmem. That or we
// make IndexUpdate a POD type.
// TODO: Instead of all of that work above, we pipe the IndexUpdate across processes as JSON.
//       We need to verify we need multiple processes first. Maybe libclang can run in a single process...
// TODO: Compute IndexUpdates in main process, off the blocking thread. Use separate process for running
//       libclang. Solves memory worries.
// TODO: Instead of passing to/from json, we can probably bass the IndexedFile type almost directly as
// a raw memory dump - the type has almost zero pointers inside of it. We could do a little bit of fixup
// so that passing from a separate process to the main db is really fast (no need to go through JSON).
/*
namespace foo2 {
  using Usr = size_t;
  struct UsrTable {
    size_t allocated;
    size_t used;
    const char* usrs[];
  };
}
*/

struct IndexUpdate {
  // File updates.
  std::vector<Usr> files_removed;
  std::vector<QueryableFile> files_added;
  std::vector<QueryableFile::OutlineUpdate> files_outline;

  // Type updates.
  std::vector<Usr> types_removed;
  std::vector<QueryableTypeDef> types_added;
  std::vector<QueryableTypeDef::DefUpdate> types_def_changed;
  std::vector<QueryableTypeDef::DerivedUpdate> types_derived;
  std::vector<QueryableTypeDef::UsesUpdate> types_uses;

  // Function updates.
  std::vector<Usr> funcs_removed;
  std::vector<QueryableFuncDef> funcs_added;
  std::vector<QueryableFuncDef::DefUpdate> funcs_def_changed;
  std::vector<QueryableFuncDef::DeclarationsUpdate> funcs_declarations;
  std::vector<QueryableFuncDef::DerivedUpdate> funcs_derived;
  std::vector<QueryableFuncDef::CallersUpdate> funcs_callers;
  std::vector<QueryableFuncDef::UsesUpdate> funcs_uses;

  // Variable updates.
  std::vector<Usr> vars_removed;
  std::vector<QueryableVarDef> vars_added;
  std::vector<QueryableVarDef::DefUpdate> vars_def_changed;
  std::vector<QueryableVarDef::UsesUpdate> vars_uses;


  // Creates a new IndexUpdate that will import |file|.
  explicit IndexUpdate(IndexedFile& file);

  // Creates an index update assuming that |previous| is already
  // in the index, so only the delta between |previous| and |current|
  // will be applied.
  IndexUpdate(IndexedFile& previous, IndexedFile& current);

  // Merges the contents of |update| into this IndexUpdate instance.
  void Merge(const IndexUpdate& update);
};



// The query database is heavily optimized for fast queries. It is stored
// in-memory.
struct QueryableDatabase {
  // Indicies between lookup vectors are related to symbols, ie, index 5 in
  // |qualified_names| matches index 5 in |symbols|.
  std::vector<std::string> qualified_names;
  std::vector<SymbolIdx> symbols;

  // Raw data storage.
  std::vector<QueryableFile> files; // File path is stored as a Usr.
  std::vector<QueryableTypeDef> types;
  std::vector<QueryableFuncDef> funcs;
  std::vector<QueryableVarDef> vars;

  // Lookup symbol based on a usr.
  std::unordered_map<Usr, SymbolIdx> usr_to_symbol;

  // Insert the contents of |update| into |db|.
  void ApplyIndexUpdate(IndexUpdate* update);

  void RemoveUsrs(const std::vector<Usr>& to_remove);
  void Import(const std::vector<QueryableFile>& defs);
  void Import(const std::vector<QueryableTypeDef>& defs);
  void Import(const std::vector<QueryableFuncDef>& defs);
  void Import(const std::vector<QueryableVarDef>& defs);
  void Update(const std::vector<QueryableTypeDef::DefUpdate>& updates);
  void Update(const std::vector<QueryableFuncDef::DefUpdate>& updates);
  void Update(const std::vector<QueryableVarDef::DefUpdate>& updates);
};



// TODO: For supporting vscode, lets'
//  - have our normal daemon system
//  - have frontend --language-server which accepts JSON RPC language server in stdin and emits language server
//    JSON in stdout. vscode extension will run the executable this way. it will connect to daemon as normal.
//    this means that vscode instance can be killed without actually killing core indexer process.
//      $ indexer --language-server
//  - maybe? have simple front end which lets user run
//      $ indexer --action references --location foo.cc:20:5
//
//
// https://github.com/Microsoft/vscode-languageserver-node/blob/master/client/src/main.ts
