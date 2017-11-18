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

struct QueryLocation {
  QueryFileId path;
  Range range;

  QueryLocation() {}  // Do not use, needed for reflect.
  QueryLocation(QueryFileId path, Range range) : path(path), range(range) {}

  QueryLocation OffsetStartColumn(int16_t offset) const {
    QueryLocation result = *this;
    result.range.start.column += offset;
    return result;
  }

  bool operator==(const QueryLocation& other) const {
    // Note: We ignore |is_interesting|.
    return path == other.path && range == other.range;
  }
  bool operator!=(const QueryLocation& other) const {
    return !(*this == other);
  }
  bool operator<(const QueryLocation& o) const {
    if (path < o.path)
      return true;
    return path == o.path && range < o.range;
  }
};
MAKE_REFLECT_STRUCT(QueryLocation, path, range);
MAKE_HASHABLE(QueryLocation, t.path, t.range);

enum class SymbolKind : int { Invalid, File, Type, Func, Var };
MAKE_REFLECT_TYPE_PROXY(SymbolKind, int);

namespace std {
template <>
struct hash<::SymbolKind> {
  size_t operator()(const ::SymbolKind& instance) const {
    return std::hash<int>()(static_cast<int>(instance));
  }
};
}  // namespace std

struct SymbolIdx {
  SymbolKind kind;
  size_t idx;

  SymbolIdx()
      : kind(SymbolKind::Invalid),
        idx((size_t)-1) {}  // Default ctor needed by stdlib. Do not use.
  SymbolIdx(SymbolKind kind, uint64_t idx) : kind(kind), idx(idx) {}

  bool operator==(const SymbolIdx& that) const {
    return kind == that.kind && idx == that.idx;
  }
  bool operator!=(const SymbolIdx& that) const { return !(*this == that); }
  bool operator<(const SymbolIdx& that) const {
    if (kind < that.kind)
      return true;
    return kind == that.kind && idx < that.idx;
  }
};
MAKE_REFLECT_STRUCT(SymbolIdx, kind, idx);
MAKE_HASHABLE(SymbolIdx, t.kind, t.idx);

struct SymbolRef {
  SymbolIdx idx;
  QueryLocation loc;

  SymbolRef() {}  // Do not use, needed for reflect.
  SymbolRef(SymbolIdx idx, QueryLocation loc) : idx(idx), loc(loc) {}

  bool operator==(const SymbolRef& that) const {
    return idx == that.idx && loc == that.loc;
  }
  bool operator!=(const SymbolRef& that) const { return !(*this == that); }
  bool operator<(const SymbolRef& that) const {
    if (idx < that.idx)
      return true;
    return idx == that.idx && loc.range.start < that.loc.range.start;
  }
};
MAKE_REFLECT_STRUCT(SymbolRef, idx, loc);

struct QueryFuncRef {
  // NOTE: id_ can be -1 if the function call is not coming from a function.
  QueryFuncId id_;
  QueryLocation loc;
  bool is_implicit = false;

  bool has_id() const { return static_cast<ssize_t>(id_.id) != -1; }

  QueryFuncRef() {}  // Do not use, needed for reflect.
  QueryFuncRef(QueryFuncId id, QueryLocation loc, bool is_implicit)
      : id_(id), loc(loc), is_implicit(is_implicit) {}

  bool operator==(const QueryFuncRef& that) const {
    return id_ == that.id_ && loc == that.loc &&
           is_implicit == that.is_implicit;
  }
  bool operator!=(const QueryFuncRef& that) const { return !(*this == that); }
  bool operator<(const QueryFuncRef& that) const {
    if (id_ < that.id_)
      return true;
    if (id_ == that.id_ && loc.range.start < that.loc.range.start)
      return true;
    return id_ == that.id_ && loc.range.start == that.loc.range.start &&
           is_implicit < that.is_implicit;
  }
};
MAKE_REFLECT_STRUCT(QueryFuncRef, id_, loc, is_implicit);

// There are two sources of reindex updates: the (single) definition of a
// symbol has changed, or one of many users of the symbol has changed.
//
// For simplicitly, if the single definition has changed, we update all of the
// associated single-owner definition data. See |Update*DefId|.
//
// If one of the many symbol users submits an update, we store the update such
// that it can be merged with other updates before actually being applied to
// the main database. See |MergeableUpdate|.

template <typename TId, typename TValue>
struct MergeableUpdate {
  // The type/func/var which is getting new usages.
  TId id;
  // Entries to add and remove.
  std::vector<TValue> to_add;
  std::vector<TValue> to_remove;

  MergeableUpdate(TId id, const std::vector<TValue>& to_add)
      : id(id), to_add(to_add) {}
  MergeableUpdate(TId id,
                  const std::vector<TValue>& to_add,
                  const std::vector<TValue>& to_remove)
      : id(id), to_add(to_add), to_remove(to_remove) {}
};
template <typename TVisitor, typename TId, typename TValue>
void Reflect(TVisitor& visitor, MergeableUpdate<TId, TValue>& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(id);
  REFLECT_MEMBER(to_add);
  REFLECT_MEMBER(to_remove);
  REFLECT_MEMBER_END();
}

struct QueryFile {
  struct Def {
    std::string path;
    // Includes in the file.
    std::vector<IndexInclude> includes;
    // Outline of the file (ie, for code lens).
    std::vector<SymbolRef> outline;
    // Every symbol found in the file (ie, for goto definition)
    std::vector<SymbolRef> all_symbols;
    // Parts of the file which are disabled.
    std::vector<Range> inactive_regions;
  };

  using DefUpdate = Def;

  optional<DefUpdate> def;
  size_t detailed_name_idx = (size_t)-1;

  QueryFile(const std::string& path) {
    def = DefUpdate();
    def->path = path;
  }
};
MAKE_REFLECT_STRUCT(QueryFile::Def,
                    path,
                    outline,
                    all_symbols,
                    inactive_regions);

struct QueryType {
  using DefUpdate = TypeDefDefinitionData<QueryTypeId,
                                          QueryFuncId,
                                          QueryVarId,
                                          QueryLocation>;
  using DerivedUpdate = MergeableUpdate<QueryTypeId, QueryTypeId>;
  using InstancesUpdate = MergeableUpdate<QueryTypeId, QueryVarId>;
  using UsesUpdate = MergeableUpdate<QueryTypeId, QueryLocation>;

  optional<DefUpdate> def;
  std::vector<QueryTypeId> derived;
  std::vector<QueryVarId> instances;
  std::vector<QueryLocation> uses;
  size_t detailed_name_idx = (size_t)-1;

  QueryType(const Usr& usr) : def(usr) {}
};

struct QueryFunc {
  using DefUpdate = FuncDefDefinitionData<QueryTypeId,
                                          QueryFuncId,
                                          QueryVarId,
                                          QueryFuncRef,
                                          QueryLocation>;
  using DeclarationsUpdate = MergeableUpdate<QueryFuncId, QueryLocation>;
  using DerivedUpdate = MergeableUpdate<QueryFuncId, QueryFuncId>;
  using CallersUpdate = MergeableUpdate<QueryFuncId, QueryFuncRef>;

  optional<DefUpdate> def;
  std::vector<QueryLocation> declarations;
  std::vector<QueryFuncId> derived;
  std::vector<QueryFuncRef> callers;
  size_t detailed_name_idx = (size_t)-1;

  QueryFunc(const Usr& usr) : def(usr) {}
};

struct QueryVar {
  using DefUpdate =
      VarDefDefinitionData<QueryTypeId, QueryFuncId, QueryVarId, QueryLocation>;
  using UsesUpdate = MergeableUpdate<QueryVarId, QueryLocation>;

  optional<DefUpdate> def;
  std::vector<QueryLocation> uses;
  size_t detailed_name_idx = (size_t)-1;

  QueryVar(const Usr& usr) : def(usr) {}
};

struct IndexUpdate {
  // Creates a new IndexUpdate based on the delta from previous to current. If
  // no delta computation should be done just pass null for previous.
  static IndexUpdate CreateDelta(const IdMap* previous_id_map,
                                 const IdMap* current_id_map,
                                 IndexFile* previous,
                                 IndexFile* current);

  // Merge |update| into this update; this can reduce overhead / index update
  // work can be parallelized.
  void Merge(const IndexUpdate& update);

  // Dump the update to a string.
  std::string ToString();

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
  IndexUpdate(const IdMap& previous_id_map,
              const IdMap& current_id_map,
              IndexFile& previous,
              IndexFile& current);
};
// NOTICE: We're not reflecting on files_removed or files_def_update, it is too
// much output when logging
MAKE_REFLECT_STRUCT(IndexUpdate,
                    types_removed,
                    types_def_update,
                    types_derived,
                    types_instances,
                    types_uses,
                    funcs_removed,
                    funcs_def_update,
                    funcs_declarations,
                    funcs_derived,
                    funcs_callers,
                    vars_removed,
                    vars_def_update,
                    vars_uses);

// The query database is heavily optimized for fast queries. It is stored
// in-memory.
struct QueryDatabase {
  // Indicies between lookup vectors are related to symbols, ie, index 5 in
  // |detailed_names| matches index 5 in |symbols|.
  std::vector<std::string> detailed_names;
  std::vector<SymbolIdx> symbols;

  // Raw data storage. Accessible via SymbolIdx instances.
  std::vector<QueryFile> files;
  std::vector<QueryType> types;
  std::vector<QueryFunc> funcs;
  std::vector<QueryVar> vars;

  // Lookup symbol based on a usr.
  // NOTE: For usr_to_file make sure to call LowerPathIfCaseInsensitive on key.
  // TODO: add type wrapper to enforce we call it
  spp::sparse_hash_map<Usr, QueryFileId> usr_to_file;
  spp::sparse_hash_map<Usr, QueryTypeId> usr_to_type;
  spp::sparse_hash_map<Usr, QueryFuncId> usr_to_func;
  spp::sparse_hash_map<Usr, QueryVarId> usr_to_var;

  // Marks the given Usrs as invalid.
  void RemoveUsrs(SymbolKind usr_kind, const std::vector<Usr>& to_remove);
  // Insert the contents of |update| into |db|.
  void ApplyIndexUpdate(IndexUpdate* update);
  void ImportOrUpdate(const std::vector<QueryFile::DefUpdate>& updates);
  void ImportOrUpdate(const std::vector<QueryType::DefUpdate>& updates);
  void ImportOrUpdate(const std::vector<QueryFunc::DefUpdate>& updates);
  void ImportOrUpdate(const std::vector<QueryVar::DefUpdate>& updates);
  void UpdateDetailedNames(size_t* qualified_name_index,
                           SymbolKind kind,
                           size_t symbol_index,
                           const std::string& name);
};

struct IdMap {
  const IdCache& local_ids;
  QueryFileId primary_file;

  IdMap(QueryDatabase* query_db, const IdCache& local_ids);

  QueryLocation ToQuery(Range range) const;
  QueryTypeId ToQuery(IndexTypeId id) const;
  QueryFuncId ToQuery(IndexFuncId id) const;
  QueryVarId ToQuery(IndexVarId id) const;
  QueryFuncRef ToQuery(IndexFuncRef ref) const;
  QueryLocation ToQuery(IndexFunc::Declaration decl) const;
  optional<QueryLocation> ToQuery(optional<Range> range) const;
  optional<QueryTypeId> ToQuery(optional<IndexTypeId> id) const;
  optional<QueryFuncId> ToQuery(optional<IndexFuncId> id) const;
  optional<QueryVarId> ToQuery(optional<IndexVarId> id) const;
  optional<QueryFuncRef> ToQuery(optional<IndexFuncRef> ref) const;
  optional<QueryLocation> ToQuery(optional<IndexFunc::Declaration> decl) const;
  std::vector<QueryLocation> ToQuery(std::vector<Range> ranges) const;
  std::vector<QueryTypeId> ToQuery(std::vector<IndexTypeId> ids) const;
  std::vector<QueryFuncId> ToQuery(std::vector<IndexFuncId> ids) const;
  std::vector<QueryVarId> ToQuery(std::vector<IndexVarId> ids) const;
  std::vector<QueryFuncRef> ToQuery(std::vector<IndexFuncRef> refs) const;
  std::vector<QueryLocation> ToQuery(
      std::vector<IndexFunc::Declaration> decls) const;

  SymbolIdx ToSymbol(IndexTypeId id) const;
  SymbolIdx ToSymbol(IndexFuncId id) const;
  SymbolIdx ToSymbol(IndexVarId id) const;

 private:
  spp::sparse_hash_map<IndexTypeId, QueryTypeId> cached_type_ids_;
  spp::sparse_hash_map<IndexFuncId, QueryFuncId> cached_func_ids_;
  spp::sparse_hash_map<IndexVarId, QueryVarId> cached_var_ids_;
};
