#pragma once

#include "indexer.h"
#include "serializer.h"

#include <sparsehash/dense_hash_map>

using Usr = std::string;

struct QueryableFile;
struct QueryableTypeDef;
struct QueryableFuncDef;
struct QueryableVarDef;

using QueryFileId = Id<QueryableFile>;
using QueryTypeId = Id<QueryableTypeDef>;
using QueryFuncId = Id<QueryableFuncDef>;
using QueryVarId = Id<QueryableVarDef>;






struct IdMap;



// TODO: in types, store refs separately from irefs. Then we can drop
// 'interesting' from location when that is cleaned up.

struct QueryableLocation {
  QueryFileId path;
  Range range;

  QueryableLocation(QueryFileId path, Range range)
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

enum class SymbolKind { Invalid, File, Type, Func, Var };
struct SymbolIdx {
  SymbolKind kind;
  size_t idx;

  SymbolIdx() : kind(SymbolKind::Invalid), idx(-1) {} // Default ctor needed by stdlib. Do not use.
  SymbolIdx(SymbolKind kind, uint64_t idx) : kind(kind), idx(idx) {}

  bool operator==(const SymbolIdx& that) const {
    return kind == that.kind && idx == that.idx;
  }
  bool operator!=(const SymbolIdx& that) const { return !(*this == that); }
  bool operator<(const SymbolIdx& that) const {
    return kind < that.kind || idx < that.idx;
  }
};

struct SymbolRef {
  SymbolIdx idx;
  QueryableLocation loc;

  SymbolRef(SymbolIdx idx, QueryableLocation loc) : idx(idx), loc(loc) {}

  bool operator==(const SymbolRef& that) const {
    return idx == that.idx && loc == that.loc;
  }
  bool operator!=(const SymbolRef& that) const { return !(*this == that); }
  bool operator<(const SymbolRef& that) const {
    return idx < that.idx && loc.range.start < that.loc.range.start;
  }
};

struct QueryFuncRef {
  QueryFuncId id;
  QueryableLocation loc;

  QueryFuncRef(QueryFuncId id, QueryableLocation loc) : id(id), loc(loc) {}

  bool operator==(const QueryFuncRef& that) const {
    return id == that.id && loc == that.loc;
  }
  bool operator!=(const QueryFuncRef& that) const { return !(*this == that); }
  bool operator<(const QueryFuncRef& that) const {
    return id < that.id && loc.range.start < that.loc.range.start;
  }
};

struct UsrRef {
  Usr usr;
  QueryableLocation loc;

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

  ReplacementUpdate(Usr usr, const std::vector<TValue>& values)
    : usr(usr), values(values) {}
};

struct QueryableFile {
  struct Def {
    Usr usr;
    // Outline of the file (ie, for code lens).
    std::vector<SymbolRef> outline;
    // Every symbol found in the file (ie, for goto definition)
    std::vector<SymbolRef> all_symbols;
  };

  using DefUpdate = Def;

  DefUpdate def;
  size_t qualified_name_idx = -1;

  QueryableFile(const Usr& usr) { def.usr = usr; }
  QueryableFile(const Def& def) : def(def) {}
  QueryableFile(const IdMap& id_map, const IndexedFile& indexed);
};

struct QueryableTypeDef {
  using DefUpdate = TypeDefDefinitionData<QueryTypeId, QueryFuncId, QueryVarId, QueryableLocation>;
  using DerivedUpdate = MergeableUpdate<QueryTypeId>;
  using InstantiationsUpdate = MergeableUpdate<QueryVarId>;
  using UsesUpdate = MergeableUpdate<QueryableLocation>;

  DefUpdate def;
  std::vector<QueryTypeId> derived;
  std::vector<QueryVarId> instantiations;
  std::vector<QueryableLocation> uses;
  size_t qualified_name_idx = -1;

  QueryableTypeDef(const Usr& usr) : def(usr) {}
  QueryableTypeDef(const DefUpdate& def) : def(def) {}
  QueryableTypeDef(const IdMap& id_map, const IndexedTypeDef& indexed);
};

struct QueryableFuncDef {
  using DefUpdate = FuncDefDefinitionData<QueryTypeId, QueryFuncId, QueryVarId, QueryFuncRef, QueryableLocation>;
  using DeclarationsUpdate = MergeableUpdate<QueryableLocation>;
  using DerivedUpdate = MergeableUpdate<QueryFuncId>;
  using CallersUpdate = MergeableUpdate<QueryFuncRef>;
  using UsesUpdate = MergeableUpdate<QueryableLocation>;

  DefUpdate def;
  std::vector<QueryableLocation> declarations;
  std::vector<QueryFuncId> derived;
  std::vector<QueryFuncRef> callers;
  std::vector<QueryableLocation> uses;
  size_t qualified_name_idx = -1;

  QueryableFuncDef(const Usr& usr) : def(usr) {}
  QueryableFuncDef(const DefUpdate& def) : def(def) {}
  QueryableFuncDef(const IdMap& id_map, const IndexedFuncDef& indexed);
};

struct QueryableVarDef {
  using DefUpdate = VarDefDefinitionData<QueryTypeId, QueryFuncId, QueryVarId, QueryableLocation>;
  using UsesUpdate = MergeableUpdate<QueryableLocation>;

  DefUpdate def;
  std::vector<QueryableLocation> uses;
  size_t qualified_name_idx = -1;

  QueryableVarDef(const Usr& usr) : def(usr) {}
  QueryableVarDef(const DefUpdate& def) : def(def) {}
  QueryableVarDef(const IdMap& id_map, const IndexedVarDef& indexed);
};

struct IndexUpdate {
  // Creates a new IndexUpdate based on the delta from previous to current. If
  // no delta computation should be done just pass null for previous.
  static IndexUpdate CreateDelta(const IdMap* previous_id_map, const IdMap* current_id_map, IndexedFile* previous, IndexedFile* current);

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
  IndexUpdate(const IdMap& previous_id_map, const IdMap& current_id_map, IndexedFile& previous, IndexedFile& current);
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
  google::dense_hash_map<Usr, SymbolIdx> usr_to_symbol;

  QueryableDatabase() {
    usr_to_symbol.set_empty_key("");
  }
  //std::unordered_map<Usr, SymbolIdx> usr_to_symbol;

  // Insert the contents of |update| into |db|.
  void ApplyIndexUpdate(IndexUpdate* update);

  void RemoveUsrs(const std::vector<Usr>& to_remove);
  void ImportOrUpdate(const std::vector<QueryableFile::DefUpdate>& updates);
  void ImportOrUpdate(const std::vector<QueryableTypeDef::DefUpdate>& updates);
  void ImportOrUpdate(const std::vector<QueryableFuncDef::DefUpdate>& updates);
  void ImportOrUpdate(const std::vector<QueryableVarDef::DefUpdate>& updates);
};




struct IdMap {
  // TODO threading model
  //  - [querydb] Create IdMap mapping from every id registered in local_ids
  //  - [indexer] Create IndexUpdate using IdMap cached state
  //  - [querydb] Apply IndexUpdate
  //
  // Then lookup in cached_* should *never* fail.

  const IdCache& local_ids;
  QueryFileId primary_file;

  IdMap(QueryableDatabase* query_db, const IdCache& local_ids);

  QueryableLocation ToQuery(Range range) const;
  QueryTypeId ToQuery(IndexTypeId id) const;
  QueryFuncId ToQuery(IndexFuncId id) const;
  QueryVarId ToQuery(IndexVarId id) const;
  QueryFuncRef ToQuery(IndexFuncRef ref) const;
  optional<QueryableLocation> ToQuery(optional<Range> range) const;
  optional<QueryTypeId> ToQuery(optional<IndexTypeId> id) const;
  optional<QueryFuncId> ToQuery(optional<IndexFuncId> id) const;
  optional<QueryVarId> ToQuery(optional<IndexVarId> id) const;
  optional<QueryFuncRef> ToQuery(optional<IndexFuncRef> ref) const;
  std::vector<QueryableLocation> ToQuery(std::vector<Range> ranges) const;
  std::vector<QueryTypeId> ToQuery(std::vector<IndexTypeId> ids) const;
  std::vector<QueryFuncId> ToQuery(std::vector<IndexFuncId> ids) const;
  std::vector<QueryVarId> ToQuery(std::vector<IndexVarId> ids) const;
  std::vector<QueryFuncRef> ToQuery(std::vector<IndexFuncRef> refs) const;

  SymbolIdx ToSymbol(IndexTypeId id) const;
  SymbolIdx ToSymbol(IndexFuncId id) const;
  SymbolIdx ToSymbol(IndexVarId id) const;
private:
  // TODO: make these type safe
  google::dense_hash_map<size_t, size_t> cached_type_ids_; // IndexTypeId -> QueryTypeId
  google::dense_hash_map<size_t, size_t> cached_func_ids_; // IndexFuncId -> QueryFuncId
  google::dense_hash_map<size_t, size_t> cached_var_ids_;  // IndexVarId  -> QueryVarId
};
