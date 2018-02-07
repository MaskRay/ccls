#pragma once

#include "indexer.h"
#include "serializer.h"

#include <sparsepp/spp.h>

#include <functional>

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

using Generation = uint32_t;

// Example use: |WithGen<Id<QueryType>>|, to mark an |Id| reference with
// generation, so that by comparising the generation with that stored in the
// referenced Query object, we can tell if the reference is stale (the
// referenced object has been deleted or reused).
template <typename T>
struct WithGen {
  Generation gen;
  T value;
  WithGen() : gen(-1) {}
  WithGen(const T& value) : gen(-1), value(value) {}
  WithGen(Generation gen, const T& value) : gen(gen), value(value) {}

  bool HasValue() const { return value.HasValue(); }
  explicit operator bool() const { return HasValue(); }

  bool operator==(const WithGen& o) const {
    return gen == o.gen && value == o.value;
  }
};

template <typename TVisitor, typename T>
void Reflect(TVisitor& visitor, WithGen<T>& value) {
  Reflect(visitor, value.value);
}

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

  bool HasValue() const { return range.HasValue(); }

  bool operator==(const QueryLocation& o) const {
    return path == o.path && range == o.range;
  }
  bool operator!=(const QueryLocation& o) const { return !(*this == o); }
  bool operator<(const QueryLocation& o) const {
    if (path != o.path)
      return path < o.path;
    return range < o.range;
  }
};
MAKE_REFLECT_STRUCT(QueryLocation, path, range);
MAKE_HASHABLE(QueryLocation, t.path, t.range);

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
  RawId idx;

  SymbolIdx()
      : kind(SymbolKind::Invalid),
        idx(RawId(-1)) {}  // Default ctor needed by stdlib. Do not use.
  SymbolIdx(SymbolKind kind, RawId idx) : kind(kind), idx(idx) {}

  bool operator==(const SymbolIdx& o) const {
    return kind == o.kind && idx == o.idx;
  }
  bool operator!=(const SymbolIdx& o) const { return !(*this == o); }
  bool operator<(const SymbolIdx& o) const {
    if (kind != o.kind)
      return kind < o.kind;
    return idx < o.idx;
  }
};
MAKE_REFLECT_STRUCT(SymbolIdx, kind, idx);
MAKE_HASHABLE(SymbolIdx, t.kind, t.idx);

struct SymbolRef {
  SymbolIdx idx;
  SymbolRole role;
  QueryLocation loc;

  SymbolRef() {}  // Do not use, needed for reflect.
  SymbolRef(SymbolIdx idx, SymbolRole role, QueryLocation loc)
      : idx(idx), role(role), loc(loc) {}

  std::tuple<SymbolIdx, SymbolRole, QueryLocation> ToTuple() const {
    return std::make_tuple(idx, role, loc);
  }
  bool operator==(const SymbolRef& o) const { return ToTuple() == o.ToTuple(); }
  bool operator!=(const SymbolRef& o) const { return !(*this == o); }
  bool operator<(const SymbolRef& o) const { return ToTuple() < o.ToTuple(); }
};
MAKE_REFLECT_STRUCT(SymbolRef, idx, loc);

struct QueryFuncRef {
  // NOTE: id_ can be -1 if the function call is not coming from a function.
  QueryFuncId id_;
  QueryLocation loc;
  bool is_implicit = false;

  bool HasValue() const { return id_.HasValue(); }

  QueryFuncRef() {}  // Do not use, needed for reflect.
  QueryFuncRef(QueryFuncId id, QueryLocation loc, bool is_implicit)
      : id_(id), loc(loc), is_implicit(is_implicit) {}

  std::tuple<QueryFuncId, QueryLocation, bool> ToTuple() const {
    return std::make_tuple(id_, loc, is_implicit);
  }
  bool operator==(const QueryFuncRef& o) const {
    return ToTuple() == o.ToTuple();
  }
  bool operator!=(const QueryFuncRef& o) const { return !(*this == o); }
  bool operator<(const QueryFuncRef& o) const {
    return ToTuple() < o.ToTuple();
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

template <typename T>
struct WithUsr {
  Usr usr;
  T value;

  WithUsr(Usr usr, const T& value) : usr(usr), value(value) {}
  WithUsr(Usr usr, T&& value) : usr(usr), value(std::move(value)) {}
};
template <typename TVisitor, typename T>
void Reflect(TVisitor& visitor, WithUsr<T>& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(usr);
  REFLECT_MEMBER(value);
  REFLECT_MEMBER_END();
}

template <typename T>
struct WithFileContent {
  T value;
  std::string file_content;

  WithFileContent(const T& value, const std::string& file_content) : value(value), file_content(file_content) {}
};
template <typename TVisitor, typename T>
void Reflect(TVisitor& visitor, WithFileContent<T>& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(value);
  REFLECT_MEMBER(file_content);
  REFLECT_MEMBER_END();
}

struct QueryFile {
  struct Def {
    std::string path;
    // Language identifier
    std::string language;
    // Includes in the file.
    std::vector<IndexInclude> includes;
    // Outline of the file (ie, for code lens).
    std::vector<SymbolRef> outline;
    // Every symbol found in the file (ie, for goto definition)
    std::vector<SymbolRef> all_symbols;
    // Parts of the file which are disabled.
    std::vector<Range> inactive_regions;
    // Used by |$cquery/freshenIndex|.
    std::vector<std::string> dependencies;
  };

  using DefUpdate = WithFileContent<Def>;

  optional<Def> def;
  Maybe<Id<void>> symbol_idx;

  explicit QueryFile(const std::string& path) {
    def = Def();
    def->path = path;
  }
};
MAKE_REFLECT_STRUCT(QueryFile::Def,
                    path,
                    language,
                    outline,
                    all_symbols,
                    inactive_regions,
                    dependencies);

struct QueryType {
  using Def = TypeDefDefinitionData<QueryTypeId,
                                    QueryFuncId,
                                    QueryVarId,
                                    QueryLocation>;
  using DefUpdate = WithUsr<Def>;
  using DerivedUpdate = MergeableUpdate<QueryTypeId, QueryTypeId>;
  using InstancesUpdate = MergeableUpdate<QueryTypeId, QueryVarId>;
  using UsesUpdate = MergeableUpdate<QueryTypeId, QueryLocation>;

  Usr usr;
  Generation gen;
  Maybe<Id<void>> symbol_idx;
  optional<Def> def;
  std::vector<QueryTypeId> derived;
  std::vector<QueryVarId> instances;
  std::vector<QueryLocation> uses;

  explicit QueryType(const Usr& usr) : usr(usr), gen(0) {}
};

struct QueryFunc {
  using Def = FuncDefDefinitionData<QueryTypeId,
                                    QueryFuncId,
                                    QueryVarId,
                                    QueryFuncRef,
                                    QueryLocation>;
  using DefUpdate = WithUsr<Def>;
  using DeclarationsUpdate = MergeableUpdate<QueryFuncId, QueryLocation>;
  using DerivedUpdate = MergeableUpdate<QueryFuncId, QueryFuncId>;
  using CallersUpdate = MergeableUpdate<QueryFuncId, QueryFuncRef>;

  Usr usr;
  Generation gen;
  Maybe<Id<void>> symbol_idx;
  optional<Def> def;
  std::vector<QueryLocation> declarations;
  std::vector<QueryFuncId> derived;
  std::vector<QueryFuncRef> callers;

  explicit QueryFunc(const Usr& usr) : usr(usr), gen(0) {}
};

struct QueryVar {
  using Def = VarDefDefinitionData<QueryTypeId,
                                   QueryFuncId,
                                   QueryVarId,
                                   QueryLocation>;
  using DefUpdate = WithUsr<Def>;
  using DeclarationsUpdate = MergeableUpdate<QueryVarId, QueryLocation>;
  using UsesUpdate = MergeableUpdate<QueryVarId, QueryLocation>;

  Usr usr;
  Generation gen;
  Maybe<Id<void>> symbol_idx;
  optional<Def> def;
  std::vector<QueryLocation> declarations;
  std::vector<QueryLocation> uses;

  explicit QueryVar(const Usr& usr) : usr(usr), gen(0) {}
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
  void Merge(IndexUpdate&& update);

  // Dump the update to a string.
  std::string ToString();

  // File updates.
  std::vector<std::string> files_removed;
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
  std::vector<QueryVar::DeclarationsUpdate> vars_declarations;
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
                    vars_declarations,
                    vars_uses);

struct NormalizedPath {
  explicit NormalizedPath(const std::string& path);
  bool operator==(const NormalizedPath& rhs) const;
  bool operator!=(const NormalizedPath& rhs) const;

  std::string path;
};
MAKE_HASHABLE(NormalizedPath, t.path);

// The query database is heavily optimized for fast queries. It is stored
// in-memory.
struct QueryDatabase {
  // All File/Func/Type/Var symbols.
  std::vector<SymbolIdx> symbols;

  // Raw data storage. Accessible via SymbolIdx instances.
  std::vector<QueryFile> files;
  std::vector<QueryType> types;
  std::vector<QueryFunc> funcs;
  std::vector<QueryVar> vars;

  // Lookup symbol based on a usr.
  spp::sparse_hash_map<NormalizedPath, QueryFileId> usr_to_file;
  spp::sparse_hash_map<Usr, QueryTypeId> usr_to_type;
  spp::sparse_hash_map<Usr, QueryFuncId> usr_to_func;
  spp::sparse_hash_map<Usr, QueryVarId> usr_to_var;

  // Marks the given Usrs as invalid.
  void RemoveUsrs(SymbolKind usr_kind, const std::vector<Usr>& to_remove);
  // Insert the contents of |update| into |db|.
  void ApplyIndexUpdate(IndexUpdate* update);
  void ImportOrUpdate(const std::vector<QueryFile::DefUpdate>& updates);
  void ImportOrUpdate(std::vector<QueryType::DefUpdate>&& updates);
  void ImportOrUpdate(std::vector<QueryFunc::DefUpdate>&& updates);
  void ImportOrUpdate(std::vector<QueryVar::DefUpdate>&& updates);
  void UpdateSymbols(Maybe<Id<void>>* symbol_idx, SymbolKind kind, RawId idx);
  std::string_view GetSymbolDetailedName(RawId symbol_idx) const;
  std::string_view GetSymbolShortName(RawId symbol_idx) const;

  // Query the indexing structure to look up symbol id for given Usr.
  Maybe<QueryFileId> GetQueryFileIdFromPath(const std::string& path);
  Maybe<QueryTypeId> GetQueryTypeIdFromUsr(Usr usr);
  Maybe<QueryFuncId> GetQueryFuncIdFromUsr(Usr usr);
  Maybe<QueryVarId> GetQueryVarIdFromUsr(Usr usr);
};

template <typename I>
struct IndexToQuery;

// clang-format off
template <> struct IndexToQuery<IndexFuncId> { using type = QueryFuncId; };
template <> struct IndexToQuery<IndexTypeId> { using type = QueryTypeId; };
template <> struct IndexToQuery<IndexVarId> { using type = QueryVarId; };
template <> struct IndexToQuery<IndexFuncRef> { using type = QueryFuncRef; };
template <> struct IndexToQuery<Range> { using type = QueryLocation; };
template <> struct IndexToQuery<IndexFunc::Declaration> { using type = QueryLocation; };
template <typename I> struct IndexToQuery<optional<I>> {
  using type = optional<typename IndexToQuery<I>::type>;
};
template <typename I> struct IndexToQuery<std::vector<I>> {
  using type = std::vector<typename IndexToQuery<I>::type>;
};
// clang-format on

struct IdMap {
  const IdCache& local_ids;
  QueryFileId primary_file;

  IdMap(QueryDatabase* query_db, const IdCache& local_ids);

  // FIXME Too verbose
  // clang-format off
  QueryLocation ToQuery(Range range) const;
  QueryTypeId ToQuery(IndexTypeId id) const;
  QueryFuncId ToQuery(IndexFuncId id) const;
  QueryVarId ToQuery(IndexVarId id) const;
  QueryFuncRef ToQuery(IndexFuncRef ref) const;
  QueryLocation ToQuery(IndexFunc::Declaration decl) const;
  template <typename I>
  Maybe<typename IndexToQuery<I>::type> ToQuery(Maybe<I> id) const {
    if (!id)
      return nullopt;
    return ToQuery(*id);
  }
  template <typename I>
  Maybe<WithGen<typename IndexToQuery<I>::type>> ToQuery(Maybe<I> id, int) const {
    if (!id)
      return nullopt;
    return ToQuery(*id, 0);
  }
  template <typename I>
  std::vector<typename IndexToQuery<I>::type> ToQuery(const std::vector<I>& a) const {
    std::vector<typename IndexToQuery<I>::type> ret;
    ret.reserve(a.size());
    for (auto& x : a)
      ret.push_back(ToQuery(x));
    return ret;
  }
  template <typename I>
  std::vector<WithGen<typename IndexToQuery<I>::type>> ToQuery(std::vector<I> a, int) const {
    std::vector<WithGen<typename IndexToQuery<I>::type>> ret;
    ret.reserve(a.size());
    for (auto& x : a)
      ret.push_back(ToQuery(x, 0));
    return ret;
  }
  // clang-format on

  SymbolIdx ToSymbol(IndexTypeId id) const;
  SymbolIdx ToSymbol(IndexFuncId id) const;
  SymbolIdx ToSymbol(IndexVarId id) const;

 private:
  spp::sparse_hash_map<IndexTypeId, QueryTypeId> cached_type_ids_;
  spp::sparse_hash_map<IndexFuncId, QueryFuncId> cached_func_ids_;
  spp::sparse_hash_map<IndexVarId, QueryVarId> cached_var_ids_;
};
