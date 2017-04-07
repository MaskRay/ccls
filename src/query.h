#pragma once

#include "indexer.h"
#include "serializer.h"

using Usr = std::string;

// TODO: in types, store refs separately from irefs. Then we can drop
// 'interesting' from location when that is cleaned up.

// TODO: Switch over to QueryableLocation. Figure out if there is
//       a good way to get the indexer using it. I don't think so
//       since we may discover more files while indexing a file.
//
//       We could also reuse planned USR caching system for file
//       paths.
struct QueryableLocation {
  Usr path;
  Range range;
  
  QueryableLocation()
    : path("") {}
  QueryableLocation(Usr path, Range range)
    : path(path), range(range) {}

  QueryableLocation OffsetStartColumn(int offset) const {
    QueryableLocation result = *this;
    result.range.start.column += offset;
    return result;
  }

  bool operator==(const QueryableLocation& other) const {
    // Note: We ignore |is_interesting|.
    return
      path == other.path &&
      range == other.range;
  }
  bool operator!=(const QueryableLocation& other) const { return !(*this == other); }
  bool operator<(const QueryableLocation& o) const {
    return
      path < o.path &&
      range < o.range;
  }
};

struct UsrRef {
  Usr usr;
  QueryableLocation loc;

  UsrRef() {}
  UsrRef(Usr usr, QueryableLocation loc) : usr(usr), loc(loc) {}

  bool operator==(const UsrRef& other) const {
    return usr == other.usr && loc.range.start == other.loc.range.start;
  }
  bool operator!=(const UsrRef& other) const { return !(*this == other); }
  bool operator<(const UsrRef& other) const {
    return usr < other.usr && loc.range.start < other.loc.range.start;
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

  MergeableUpdate(Usr usr, const std::vector<TValue>& to_add)
    : usr(usr), to_add(to_add) {}
  MergeableUpdate(Usr usr, const std::vector<TValue>& to_add, const std::vector<TValue>& to_remove)
    : usr(usr), to_add(to_add), to_remove(to_remove) {}
};

template<typename TValue>
struct ReplacementUpdate {
  // The type/func/var which is getting new usages.
  Usr usr;
  // New entries.
  std::vector<TValue> values;

  ReplacementUpdate(Usr usr, const std::vector<TValue>& entries)
    : usr(usr), entries(entries) {}
};

struct QueryableFile {
  struct Def {
    Usr usr;
    // Outline of the file (ie, for code lens).
    std::vector<UsrRef> outline;
    // Every symbol found in the file (ie, for goto definition)
    std::vector<UsrRef> all_symbols;
  };

  using DefUpdate = Def;

  DefUpdate def;

  QueryableFile() {}
  QueryableFile(const IndexedFile& indexed);
};

struct QueryableTypeDef {
  using DefUpdate = TypeDefDefinitionData<Usr, Usr, Usr, QueryableLocation, QueryableLocation>;
  using DerivedUpdate = MergeableUpdate<Usr>;
  using InstantiationsUpdate = MergeableUpdate<Usr>;
  using UsesUpdate = MergeableUpdate<QueryableLocation>;

  DefUpdate def;
  std::vector<Usr> derived;
  std::vector<Usr> instantiations;
  std::vector<QueryableLocation> uses;

  QueryableTypeDef() : def("") {}
  QueryableTypeDef(IdCache& id_cache, const IndexedTypeDef& indexed);
};

struct QueryableFuncDef {
  using DefUpdate = FuncDefDefinitionData<Usr, Usr, Usr, UsrRef, QueryableLocation, QueryableLocation>;
  using DeclarationsUpdate = MergeableUpdate<QueryableLocation>;
  using DerivedUpdate = MergeableUpdate<Usr>;
  using CallersUpdate = MergeableUpdate<UsrRef>;
  using UsesUpdate = MergeableUpdate<QueryableLocation>;

  DefUpdate def;
  std::vector<QueryableLocation> declarations;
  std::vector<Usr> derived;
  std::vector<UsrRef> callers;
  std::vector<QueryableLocation> uses;

  QueryableFuncDef() : def("") {}
  QueryableFuncDef(IdCache& id_cache, const IndexedFuncDef& indexed);
};

struct QueryableVarDef {
  using DefUpdate = VarDefDefinitionData<Usr, Usr, Usr, QueryableLocation, QueryableLocation>;
  using UsesUpdate = MergeableUpdate<QueryableLocation>;

  DefUpdate def;
  std::vector<QueryableLocation> uses;

  QueryableVarDef() : def("") {}
  QueryableVarDef(IdCache& id_cache, const IndexedVarDef& indexed);
};

enum class SymbolKind { Invalid, File, Type, Func, Var };
struct SymbolIdx {
  SymbolKind kind;
  uint64_t idx;

  SymbolIdx() : kind(SymbolKind::Invalid), idx(-1) {} // Default ctor needed by stdlib. Do not use.
  SymbolIdx(SymbolKind kind, uint64_t idx) : kind(kind), idx(idx) {}
};


struct IndexUpdate {
  // Creates a new IndexUpdate that will import |file|.
  static IndexUpdate CreateImport(IndexedFile& file);
  static IndexUpdate CreateDelta(IndexedFile& current, IndexedFile& updated);

  // Merge |update| into this update; this can reduce overhead / index update
  // work can be parallelized.
  void Merge(const IndexUpdate& update);

  // File updates.
  std::vector<Usr> files_removed;
  std::vector<QueryableFile::DefUpdate> files_def_update;

  // Type updates.
  std::vector<Usr> types_removed;
  std::vector<QueryableTypeDef::DefUpdate> types_def_update;
  std::vector<QueryableTypeDef::DerivedUpdate> types_derived;
  std::vector<QueryableTypeDef::InstantiationsUpdate> types_instantiations;
  std::vector<QueryableTypeDef::UsesUpdate> types_uses;

  // Function updates.
  std::vector<Usr> funcs_removed;
  std::vector<QueryableFuncDef::DefUpdate> funcs_def_update;
  std::vector<QueryableFuncDef::DeclarationsUpdate> funcs_declarations;
  std::vector<QueryableFuncDef::DerivedUpdate> funcs_derived;
  std::vector<QueryableFuncDef::CallersUpdate> funcs_callers;
  std::vector<QueryableFuncDef::UsesUpdate> funcs_uses;

  // Variable updates.
  std::vector<Usr> vars_removed;
  std::vector<QueryableVarDef::DefUpdate> vars_def_update;
  std::vector<QueryableVarDef::UsesUpdate> vars_uses;

 private:
  // Creates an index update assuming that |previous| is already
  // in the index, so only the delta between |previous| and |current|
  // will be applied.
  IndexUpdate(IndexedFile& previous, IndexedFile& current);
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
  void ImportOrUpdate(const std::vector<QueryableFile::DefUpdate>& updates);
  void ImportOrUpdate(const std::vector<QueryableTypeDef::DefUpdate>& updates);
  void ImportOrUpdate(const std::vector<QueryableFuncDef::DefUpdate>& updates);
  void ImportOrUpdate(const std::vector<QueryableVarDef::DefUpdate>& updates);
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
