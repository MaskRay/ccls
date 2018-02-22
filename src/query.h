#pragma once

#include "indexer.h"
#include "serializer.h"

#include <sparsepp/spp.h>

#include <forward_list>
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

  MergeableUpdate(TId id, std::vector<TValue>&& to_add)
      : id(id), to_add(std::move(to_add)) {}
  MergeableUpdate(TId id,
                  std::vector<TValue>&& to_add,
                  std::vector<TValue>&& to_remove)
      : id(id), to_add(std::move(to_add)), to_remove(std::move(to_remove)) {}
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

  WithFileContent(const T& value, const std::string& file_content)
      : value(value), file_content(file_content) {}
};
template <typename TVisitor, typename T>
void Reflect(TVisitor& visitor, WithFileContent<T>& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(value);
  REFLECT_MEMBER(file_content);
  REFLECT_MEMBER_END();
}

struct QueryFamily {
  using FileId = Id<QueryFile>;
  using FuncId = Id<QueryFunc>;
  using TypeId = Id<QueryType>;
  using VarId = Id<QueryVar>;
  using Range = Reference;
};

struct QueryFile {
  struct Def {
    Id<QueryFile> file;
    std::string path;
    std::vector<std::string> args;
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
                    file,
                    path,
                    args,
                    language,
                    outline,
                    all_symbols,
                    inactive_regions,
                    dependencies);

template <typename Q, typename QDef>
struct QueryEntity {
  using Def = QDef;
  using DefUpdate = WithUsr<Def>;
  using DeclarationsUpdate = MergeableUpdate<Id<Q>, Use>;
  using UsesUpdate = MergeableUpdate<Id<Q>, Use>;
  Def* AnyDef() {
    Def* ret = nullptr;
    for (auto& i : static_cast<Q*>(this)->def) {
      ret = &i;
      if (i.spell)
        break;
    }
    return ret;
  }
  const Def* AnyDef() const { return const_cast<QueryEntity*>(this)->AnyDef(); }
};

struct QueryType : QueryEntity<QueryType, TypeDefDefinitionData<QueryFamily>> {
  using DerivedUpdate = MergeableUpdate<QueryTypeId, QueryTypeId>;
  using InstancesUpdate = MergeableUpdate<QueryTypeId, QueryVarId>;

  Usr usr;
  Maybe<Id<void>> symbol_idx;
  std::forward_list<Def> def;
  std::vector<Use> declarations;
  std::vector<QueryTypeId> derived;
  std::vector<QueryVarId> instances;
  std::vector<Use> uses;

  explicit QueryType(const Usr& usr) : usr(usr) {}
};

struct QueryFunc : QueryEntity<QueryFunc, FuncDefDefinitionData<QueryFamily>> {
  using DerivedUpdate = MergeableUpdate<QueryFuncId, QueryFuncId>;

  Usr usr;
  Maybe<Id<void>> symbol_idx;
  std::forward_list<Def> def;
  std::vector<Use> declarations;
  std::vector<QueryFuncId> derived;
  std::vector<Use> uses;

  explicit QueryFunc(const Usr& usr) : usr(usr) {}
};

struct QueryVar : QueryEntity<QueryVar, VarDefDefinitionData<QueryFamily>> {
  Usr usr;
  Maybe<Id<void>> symbol_idx;
  std::forward_list<Def> def;
  std::vector<Use> declarations;
  std::vector<Use> uses;

  explicit QueryVar(const Usr& usr) : usr(usr) {}
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
  std::vector<QueryType::DeclarationsUpdate> types_declarations;
  std::vector<QueryType::DerivedUpdate> types_derived;
  std::vector<QueryType::InstancesUpdate> types_instances;
  std::vector<QueryType::UsesUpdate> types_uses;

  // Function updates.
  std::vector<WithUsr<QueryFileId>> funcs_removed;
  std::vector<QueryFunc::DefUpdate> funcs_def_update;
  std::vector<QueryFunc::DeclarationsUpdate> funcs_declarations;
  std::vector<QueryFunc::DerivedUpdate> funcs_derived;
  std::vector<QueryFunc::UsesUpdate> funcs_uses;

  // Variable updates.
  std::vector<WithUsr<QueryFileId>> vars_removed;
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
                    funcs_uses,
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
  void RemoveUsrs(SymbolKind usr_kind,
                  const std::vector<WithUsr<QueryFileId>>& to_remove);
  // Insert the contents of |update| into |db|.
  void ApplyIndexUpdate(IndexUpdate* update);
  void ImportOrUpdate(const std::vector<QueryFile::DefUpdate>& updates);
  void ImportOrUpdate(std::vector<QueryType::DefUpdate>&& updates);
  void ImportOrUpdate(std::vector<QueryFunc::DefUpdate>&& updates);
  void ImportOrUpdate(std::vector<QueryVar::DefUpdate>&& updates);
  void UpdateSymbols(Maybe<Id<void>>* symbol_idx,
                     SymbolKind kind,
                     Id<void> idx);
  std::string_view GetSymbolDetailedName(RawId symbol_idx) const;
  std::string_view GetSymbolShortName(RawId symbol_idx) const;

  // Query the indexing structure to look up symbol id for given Usr.
  Maybe<QueryFileId> GetQueryFileIdFromPath(const std::string& path);
  Maybe<QueryTypeId> GetQueryTypeIdFromUsr(Usr usr);
  Maybe<QueryFuncId> GetQueryFuncIdFromUsr(Usr usr);
  Maybe<QueryVarId> GetQueryVarIdFromUsr(Usr usr);

  QueryFile& GetFile(SymbolIdx ref) { return files[ref.id.id]; }
  QueryFunc& GetFunc(SymbolIdx ref) { return funcs[ref.id.id]; }
  QueryType& GetType(SymbolIdx ref) { return types[ref.id.id]; }
  QueryVar& GetVar(SymbolIdx ref) { return vars[ref.id.id]; }
};

template <typename I>
struct IndexToQuery;

// clang-format off
template <> struct IndexToQuery<IndexFileId> { using type = QueryFileId; };
template <> struct IndexToQuery<IndexFuncId> { using type = QueryFuncId; };
template <> struct IndexToQuery<IndexTypeId> { using type = QueryTypeId; };
template <> struct IndexToQuery<IndexVarId> { using type = QueryVarId; };
template <> struct IndexToQuery<Use> { using type = Use; };
template <> struct IndexToQuery<SymbolRef> { using type = SymbolRef; };
template <> struct IndexToQuery<IndexFunc::Declaration> { using type = Use; };
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
  QueryTypeId ToQuery(IndexTypeId id) const;
  QueryFuncId ToQuery(IndexFuncId id) const;
  QueryVarId ToQuery(IndexVarId id) const;
  SymbolRef ToQuery(SymbolRef ref) const;
  Use ToQuery(Reference ref) const;
  Use ToQuery(Use ref) const;
  Use ToQuery(IndexFunc::Declaration decl) const;
  template <typename I>
  Maybe<typename IndexToQuery<I>::type> ToQuery(Maybe<I> id) const {
    if (!id)
      return nullopt;
    return ToQuery(*id);
  }
  template <typename I>
  std::vector<typename IndexToQuery<I>::type> ToQuery(const std::vector<I>& a) const {
    std::vector<typename IndexToQuery<I>::type> ret;
    ret.reserve(a.size());
    for (auto& x : a)
      ret.push_back(ToQuery(x));
    return ret;
  }
  // clang-format on

 private:
  spp::sparse_hash_map<IndexTypeId, QueryTypeId> cached_type_ids_;
  spp::sparse_hash_map<IndexFuncId, QueryFuncId> cached_func_ids_;
  spp::sparse_hash_map<IndexVarId, QueryVarId> cached_var_ids_;
};
