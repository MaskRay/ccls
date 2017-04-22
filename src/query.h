#pragma once

#include "indexer.h"
#include "serializer.h"

#include <sparsepp/spp.h>

#include <functional>

using Usr = std::string;

struct QueryFile;
struct QueryType;
struct QueryFunc;
struct QueryVar;
struct QueryDatabase;

using QueryFileId = Id<QueryFile>;
using QueryTypeId = Id<QueryType>;
using QueryFuncId = Id<QueryFunc>;
using QueryVarId = Id<QueryVar>;






struct IdMap;



// TODO: in types, store refs separately from irefs. Then we can drop
// 'interesting' from location when that is cleaned up.

struct QueryLocation {
  QueryFileId path;
  Range range;

  QueryLocation(QueryFileId path, Range range)
    : path(path), range(range) {}

  QueryLocation OffsetStartColumn(int offset) const {
    QueryLocation result = *this;
    result.range.start.column += offset;
    return result;
  }

  bool operator==(const QueryLocation& other) const {
    // Note: We ignore |is_interesting|.
    return
      path == other.path &&
      range == other.range;
  }
  bool operator!=(const QueryLocation& other) const { return !(*this == other); }
  bool operator<(const QueryLocation& o) const {
    return
      path < o.path &&
      range < o.range;
  }
};

enum class SymbolKind { Invalid, File, Type, Func, Var };
struct SymbolIdx {
  SymbolKind kind;
  size_t idx;

  explicit SymbolIdx() : kind(SymbolKind::Invalid), idx(-1) {} // Default ctor needed by stdlib. Do not use.
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
  QueryLocation loc;

  SymbolRef(SymbolIdx idx, QueryLocation loc) : idx(idx), loc(loc) {}

  bool operator==(const SymbolRef& that) const {
    return idx == that.idx && loc == that.loc;
  }
  bool operator!=(const SymbolRef& that) const { return !(*this == that); }
  bool operator<(const SymbolRef& that) const {
    return idx < that.idx && loc.range.start < that.loc.range.start;
  }
};

struct QueryFuncRef {
  QueryFuncId id() const {
    assert(has_id());
    return id_;
  }
  bool has_id() const {
    return id_.id != -1;
  }

  QueryFuncId id_;
  QueryLocation loc;

  QueryFuncRef(QueryFuncId id, QueryLocation loc) : id_(id), loc(loc) {}

  bool operator==(const QueryFuncRef& that) const {
    return id_ == that.id_ && loc == that.loc;
  }
  bool operator!=(const QueryFuncRef& that) const { return !(*this == that); }
  bool operator<(const QueryFuncRef& that) const {
    return id_ < that.id_ && loc.range.start < that.loc.range.start;
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

template<typename TId, typename TValue>
struct MergeableUpdate {
  // The type/func/var which is getting new usages.
  TId id;
  // Entries to add and remove.
  std::vector<TValue> to_add;
  std::vector<TValue> to_remove;

  MergeableUpdate(TId id, const std::vector<TValue>& to_add)
    : id(id), to_add(to_add) {}
  MergeableUpdate(TId id, const std::vector<TValue>& to_add, const std::vector<TValue>& to_remove)
    : id(id), to_add(to_add), to_remove(to_remove) {}
};

struct QueryFile {
  struct Def {
    std::string path;
    // Outline of the file (ie, for code lens).
    std::vector<SymbolRef> outline;
    // Every symbol found in the file (ie, for goto definition)
    std::vector<SymbolRef> all_symbols;
  };

  using DefUpdate = Def;

  DefUpdate def;
  size_t detailed_name_idx = -1;

  QueryFile(const std::string& path) { def.path = path; }
};

struct QueryType {
  using DefUpdate = TypeDefDefinitionData<QueryTypeId, QueryFuncId, QueryVarId, QueryLocation>;
  using DerivedUpdate = MergeableUpdate<QueryTypeId, QueryTypeId>;
  using InstancesUpdate = MergeableUpdate<QueryTypeId, QueryVarId>;
  using UsesUpdate = MergeableUpdate<QueryTypeId, QueryLocation>;

  DefUpdate def;
  std::vector<QueryTypeId> derived;
  std::vector<QueryVarId> instances;
  std::vector<QueryLocation> uses;
  size_t detailed_name_idx = -1;

  QueryType(const Usr& usr) : def(usr) {}
};

struct QueryFunc {
  using DefUpdate = FuncDefDefinitionData<QueryTypeId, QueryFuncId, QueryVarId, QueryFuncRef, QueryLocation>;
  using DeclarationsUpdate = MergeableUpdate<QueryFuncId, QueryLocation>;
  using DerivedUpdate = MergeableUpdate<QueryFuncId, QueryFuncId>;
  using CallersUpdate = MergeableUpdate<QueryFuncId, QueryFuncRef>;

  DefUpdate def;
  std::vector<QueryLocation> declarations;
  std::vector<QueryFuncId> derived;
  std::vector<QueryFuncRef> callers;
  size_t detailed_name_idx = -1;

  QueryFunc(const Usr& usr) : def(usr) {}
};

struct QueryVar {
  using DefUpdate = VarDefDefinitionData<QueryTypeId, QueryFuncId, QueryVarId, QueryLocation>;
  using UsesUpdate = MergeableUpdate<QueryVarId, QueryLocation>;

  DefUpdate def;
  std::vector<QueryLocation> uses;
  size_t detailed_name_idx = -1;

  QueryVar(const Usr& usr) : def(usr) {}
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
  std::vector<QueryFile::DefUpdate> files_def_update;

  // Type updates.
  std::vector<Usr> types_removed;
  std::vector<QueryType::DefUpdate> types_def_update;
  std::vector<QueryType::DerivedUpdate> types_derived;
  std::vector<QueryType::InstancesUpdate> types_instances;
  std::vector<QueryType::UsesUpdate> types_uses;

  // Function updates.
  std::vector<Usr> funcs_removed;
  std::vector<QueryFunc::DefUpdate> funcs_def_update;
  std::vector<QueryFunc::DeclarationsUpdate> funcs_declarations;
  std::vector<QueryFunc::DerivedUpdate> funcs_derived;
  std::vector<QueryFunc::CallersUpdate> funcs_callers;

  // Variable updates.
  std::vector<Usr> vars_removed;
  std::vector<QueryVar::DefUpdate> vars_def_update;
  std::vector<QueryVar::UsesUpdate> vars_uses;

 private:
  // Creates an index update assuming that |previous| is already
  // in the index, so only the delta between |previous| and |current|
  // will be applied.
  IndexUpdate(const IdMap& previous_id_map, const IdMap& current_id_map, IndexedFile& previous, IndexedFile& current);
};


// The query database is heavily optimized for fast queries. It is stored
// in-memory.
struct QueryDatabase {
  // Indicies between lookup vectors are related to symbols, ie, index 5 in
  // |detailed_names| matches index 5 in |symbols|.
  std::vector<std::string> detailed_names;
  std::vector<SymbolIdx> symbols;

  // Raw data storage. Accessible via SymbolIdx instances.
  std::vector<optional<QueryFile>> files;
  std::vector<optional<QueryType>> types;
  std::vector<optional<QueryFunc>> funcs;
  std::vector<optional<QueryVar>> vars;

  // Lookup symbol based on a usr.
  spp::sparse_hash_map<Usr, SymbolIdx> usr_to_symbol;

  // Marks the given Usrs as invalid.
  void RemoveUsrs(const std::vector<Usr>& to_remove);
  // Insert the contents of |update| into |db|.
  void ApplyIndexUpdate(IndexUpdate* update);
  void ImportOrUpdate(const std::vector<QueryFile::DefUpdate>& updates);
  void ImportOrUpdate(const std::vector<QueryType::DefUpdate>& updates);
  void ImportOrUpdate(const std::vector<QueryFunc::DefUpdate>& updates);
  void ImportOrUpdate(const std::vector<QueryVar::DefUpdate>& updates);
  void UpdateDetailedNames(size_t* qualified_name_index, SymbolKind kind, size_t symbol_index, const std::string& name);
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

  IdMap(QueryDatabase* query_db, const IdCache& local_ids);

  QueryLocation ToQuery(Range range) const;
  QueryTypeId ToQuery(IndexTypeId id) const;
  QueryFuncId ToQuery(IndexFuncId id) const;
  QueryVarId ToQuery(IndexVarId id) const;
  QueryFuncRef ToQuery(IndexFuncRef ref) const;
  optional<QueryLocation> ToQuery(optional<Range> range) const;
  optional<QueryTypeId> ToQuery(optional<IndexTypeId> id) const;
  optional<QueryFuncId> ToQuery(optional<IndexFuncId> id) const;
  optional<QueryVarId> ToQuery(optional<IndexVarId> id) const;
  optional<QueryFuncRef> ToQuery(optional<IndexFuncRef> ref) const;
  std::vector<QueryLocation> ToQuery(std::vector<Range> ranges) const;
  std::vector<QueryTypeId> ToQuery(std::vector<IndexTypeId> ids) const;
  std::vector<QueryFuncId> ToQuery(std::vector<IndexFuncId> ids) const;
  std::vector<QueryVarId> ToQuery(std::vector<IndexVarId> ids) const;
  std::vector<QueryFuncRef> ToQuery(std::vector<IndexFuncRef> refs) const;

  SymbolIdx ToSymbol(IndexTypeId id) const;
  SymbolIdx ToSymbol(IndexFuncId id) const;
  SymbolIdx ToSymbol(IndexVarId id) const;
private:
  spp::sparse_hash_map<IndexTypeId, QueryTypeId> cached_type_ids_;
  spp::sparse_hash_map<IndexFuncId, QueryFuncId> cached_func_ids_;
  spp::sparse_hash_map<IndexVarId, QueryVarId> cached_var_ids_;
};
