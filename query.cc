#include "query.h"

#include <cstdint>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <iostream>

#include "function_output_iterator.hpp"
#include "compilation_database_loader.h"
#include "optional.h"
#include "indexer.h"

//#define CATCH_CONFIG_MAIN
//#include "catch.hpp"

// TODO: Make all copy constructors explicit.

struct IdMap {
  // The first vector is indexed by TId::group.
  // The second vector is indexed by TId::id.
  template<typename TId>
  using GroupMap = std::vector<std::unordered_map<TId, TId>>;

  GroupId target_group;
  int64_t next_file_id = 1;
  int64_t next_type_id = 1;
  int64_t next_func_id = 1;
  int64_t next_var_id = 1;

  GroupMap<FileId> remap_file_id;
  GroupMap<TypeId> remap_type_id;
  GroupMap<FuncId> remap_func_id;
  GroupMap<VarId> remap_var_id;

  IdMap(GroupId target_group) : target_group(target_group) {}

  template<typename TId>
  inline TId GenericRemap(GroupMap<TId>* map, int64_t* next_id, TId from) {
    // PERF: If this function is a hot-spot we can pull the group computation
    // out, ie,
    //
    //    IdMap id_map;
    //    GroupIdMap group_map = id_map.ResolveIdGroup(file.group)
    //    for (...)
    //      group_map.Remap(id)

    // Find the group that |from| belongs to. Create groups if needed.
    if (from.group >= map->size())
      map->resize(from.group + 1);

    // If the group doesn't have an ID already mapped out for |from|, map it.
    /*
    // TODO: The concern with this approach is that it going to waste huge
    // amounts of memory, because the first 16k+ ids can be unused.
    std::vector<TId>& group = (*map)[from.group];

    if (from.id >= group.size()) {
    group.reserve(from.id + 1);
    for (size_t i = group.size(); i < from.id; ++i)
    group.emplace_back(TId(target_group, (*next_id)++));
    }
    */

    std::unordered_map<TId, TId> group = (*map)[from.group];

    // Lookup the id from the group or add it.
    auto it = group.find(from);
    if (it == group.end()) {
      TId result(target_group, (*next_id)++);
      group[from] = result;
      return result;
    }
    return it->second;
  }

  template<typename TId>
  inline std::vector<TId> GenericVectorRemap(GroupMap<TId>* map, int64_t* next_id, const std::vector<TId>& from) {
    if (from.empty())
      return {};

    int group_id = from[0].group;
    if (group_id >= map->size())
      map->resize(group_id + 1);

    std::unordered_map<TId, TId> group = (*map)[group_id];

    std::vector<TId> result;
    result.reserve(from.size());
    for (TId id : from) {
      // Lookup the id from the group or add it.
      auto it = group.find(id);
      if (it == group.end()) {
        TId new_id(target_group, (*next_id)++);
        group[id] = new_id;
        result.push_back(new_id);
      }
      else {
        result.push_back(it->second);
      }
    }

    return result;
  }

  FileId Remap(FileId from) {
    return GenericRemap(&remap_file_id, &next_file_id, from);
  }
  Location Remap(Location from) {
    FileId file = Remap(from.file_id());
    from.raw_file_group = file.group;
    from.raw_file_id = file.id;
    return from;
  }
  TypeId Remap(TypeId from) {
    return GenericRemap(&remap_type_id, &next_type_id, from);
  }
  FuncId Remap(FuncId from) {
    return GenericRemap(&remap_func_id, &next_func_id, from);
  }
  VarId Remap(VarId from) {
    return GenericRemap(&remap_var_id, &next_var_id, from);
  }
  FuncRef Remap(FuncRef from) {
    from.id = Remap(from.id);
    from.loc = Remap(from.loc);
    return from;
  }
  TypeDefDefinitionData Remap(TypeDefDefinitionData  def) {
    def.id = Remap(def.id);
    if (def.definition)
      def.definition = Remap(def.definition.value());
    if (def.alias_of)
      def.alias_of = Remap(def.alias_of.value());
    def.parents = Remap(def.parents);
    def.types = Remap(def.types);
    def.funcs = Remap(def.funcs);
    def.vars = Remap(def.vars);
    return def;
  }
  FuncDefDefinitionData Remap(FuncDefDefinitionData def) {
    def.id = Remap(def.id);
    if (def.definition)
      def.definition = Remap(def.definition.value());
    if (def.declaring_type)
      def.declaring_type = Remap(def.declaring_type.value());
    if (def.base)
      def.base = Remap(def.base.value());
    def.locals = Remap(def.locals);
    def.callees = Remap(def.callees);
    return def;
  }
  VarDefDefinitionData Remap(VarDefDefinitionData def) {
    def.id = Remap(def.id);
    if (def.declaration)
      def.declaration = Remap(def.declaration.value());
    if (def.definition)
      def.definition = Remap(def.definition.value());
    if (def.variable_type)
      def.variable_type = Remap(def.variable_type.value());
    if (def.declaring_type)
      def.declaring_type = Remap(def.declaring_type.value());
    return def;
  }


  //std::vector<FileId> Remap(const std::vector<FileId>& from) {
  //  return GenericVectorRemap(&remap_file_id, &next_file_id, from);
  //}
  std::vector<Location> Remap(const std::vector<Location>& from) {
    std::vector<Location> result;
    result.reserve(from.size());
    for (Location l : from)
      result.push_back(Remap(l));
    return result;
  }
  std::vector<TypeId> Remap(const std::vector<TypeId>& from) {
    return GenericVectorRemap(&remap_type_id, &next_type_id, from);
  }
  std::vector<FuncId> Remap(const std::vector<FuncId>& from) {
    return GenericVectorRemap(&remap_func_id, &next_func_id, from);
  }
  std::vector<VarId> Remap(const std::vector<VarId>& from) {
    return GenericVectorRemap(&remap_var_id, &next_var_id, from);
  }
  std::vector<FuncRef> Remap(const std::vector<FuncRef>& from) {
    std::vector<FuncRef> result;
    result.reserve(from.size());
    for (FuncRef r : from)
      result.push_back(Remap(r));
    return result;
  }
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

template<typename TId, typename TValue>
MergeableUpdate<TId, TValue> MakeMergeableUpdate(IdMap* id_map, TId symbol_id, const std::vector<TValue>& removed, const std::vector<TValue>& added) {
  MergeableUpdate<TId, TValue> update;
  update.id = id_map->Remap(symbol_id);
  update.to_remove = id_map->Remap(removed);
  update.to_add = id_map->Remap(added);
  return update;
}

// NOTE: When not inside of a |def| object, there can be duplicates of the same
//       information if that information is contributed from separate sources.
//       If we need to avoid this duplication in the future, we will have to
//       add a refcount.

struct QueryableTypeDef {
  TypeDefDefinitionData def;
  std::vector<TypeId> derived;
  std::vector<Location> uses;

  using DefUpdate = TypeDefDefinitionData;
  using DerivedUpdate = MergeableUpdate<TypeId, TypeId>;
  using UsesUpdate = MergeableUpdate<TypeId, Location>;

  QueryableTypeDef(IdMap& id_map, const IndexedTypeDef& indexed)
    : def(id_map.Remap(indexed.def)) {
    derived = id_map.Remap(indexed.derived);
    uses = id_map.Remap(indexed.uses);
  }
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

  QueryableFuncDef(IdMap& id_map, const IndexedFuncDef& indexed)
    : def(id_map.Remap(indexed.def)) {
    declarations = id_map.Remap(indexed.declarations);
    derived = id_map.Remap(indexed.derived);
    callers = id_map.Remap(indexed.callers);
    uses = id_map.Remap(indexed.uses);
  }
};

struct QueryableVarDef {
  VarDefDefinitionData def;
  std::vector<Location> uses;

  using DefUpdate = VarDefDefinitionData;
  using UsesUpdate = MergeableUpdate<VarId, Location>;

  QueryableVarDef(IdMap& id_map, const IndexedVarDef& indexed)
    : def(id_map.Remap(indexed.def)) {
    uses = id_map.Remap(indexed.uses);
  }
};

struct QueryableFile {
  FileId file_id;

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
  IdMap id_map;

  // Indicies between lookup vectors are related to symbols, ie, index 5 in
  // |qualified_names| matches index 5 in |symbols|.
  std::vector<QueryableEntry> qualified_names;
  std::vector<SymbolIdx> symbols;

  // Raw data storage.
  std::vector<QueryableTypeDef> types;
  std::vector<QueryableFuncDef> funcs;
  std::vector<QueryableVarDef> vars;

  // |files| is indexed by FileId. Retrieve a FileId from a path using
  // |file_db|.
  FileDb file_db;
  std::vector<QueryableFile> files;

  // When importing data into the global db we need to remap ids from an
  // arbitrary group into the global group.
  IdMap local_id_group_to_global_id_group;
};











struct CachedIndexedFile {
  // Path to the file indexed.
  std::string path;
  // GroupId of the indexed file.
  GroupId group;

  // TODO: Make sure that |previous_index| and |current_index| use the same id
  // to USR mapping. This lets us greatly speed up difference computation.

  // The previous index. This is used for index updates, so we only apply a
  // an update diff when changing the global db.
  optional<IndexedFile> previous_index;
  IndexedFile current_index;

  CachedIndexedFile(const IndexedFile& indexed)
    : group(indexed.usr_to_id->group), current_index(indexed) {}
};


struct IndexUpdate {
  IdMap* id_map;

  // Type updates.
  std::vector<TypeId> types_removed;
  std::vector<QueryableTypeDef> types_added;
  std::vector<QueryableTypeDef::DefUpdate> types_def_changed;
  std::vector<QueryableTypeDef::DerivedUpdate> types_derived;
  std::vector<QueryableTypeDef::UsesUpdate> types_uses;

  // Function updates.
  std::vector<FuncId> funcs_removed;
  std::vector<QueryableFuncDef> funcs_added;
  std::vector<QueryableFuncDef::DefUpdate> funcs_def_changed;
  std::vector<QueryableFuncDef::DeclarationsUpdate> funcs_declarations;
  std::vector<QueryableFuncDef::DerivedUpdate> funcs_derived;
  std::vector<QueryableFuncDef::CallersUpdate> funcs_callers;
  std::vector<QueryableFuncDef::UsesUpdate> funcs_uses;

  // Variable updates.
  std::vector<VarId> vars_removed;
  std::vector<QueryableVarDef> vars_added;
  std::vector<QueryableVarDef::DefUpdate> vars_def_changed;
  std::vector<QueryableVarDef::UsesUpdate> vars_uses;

  IndexUpdate(IdMap* id_map) : id_map(id_map) {}
};


template<typename TValue>
TValue* TryFind(std::unordered_set<TValue*>& set, TValue* value) {
  // TODO: Make |value| a const ref?
  auto it = set.find(value);
  if (it == set.end())
    return nullptr;
  return *it;
}

template<typename T>
std::unordered_set<T*> CreateSet(std::vector<T>& elements) {
  std::unordered_set<T*> result;
  result.reserve(elements.size());
  for (T& element : elements)
    result.insert(&element);
  return result;
}


// Compares |previous| and |current|, adding all elements that are
// in |previous| but not |current| to |removed|, and all elements
// that are in |current| but not |previous| to |added|.
//
// Returns true iff |removed| or |added| are non-empty.
template<typename T>
bool ComputeDifferenceForUpdate(
  std::vector<T>& previous, std::vector<T>& current,
  std::vector<T>* removed, std::vector<T>* added) {

  // We need to sort to use std::set_difference.
  std::sort(previous.begin(), previous.end());
  std::sort(current.begin(), current.end());

  // Returns the elements in |previous| that are not in |current|.
  std::set_difference(
    previous.begin(), previous.end(),
    current.begin(), current.end(),
    std::back_inserter(*removed));
  // Returns the elmeents in |current| that are not in |previous|.
  std::set_difference(
    current.begin(), current.end(),
    previous.begin(), previous.end(),
    std::back_inserter(*added));

  return !removed->empty() || !added->empty();
}

#if false
template<typename T>
void CompareGroups(
  std::vector<T>& previous_data, std::vector<T>& current_data,
  std::function<void(T*)> on_removed, std::function<void(T*)> on_added, std::function<void(T*, T*)> on_found) {
  // TODO: It could be faster to use set_intersection and set_difference to
  //       compute these values. We will have to presort the input by ID, though.

  // Precompute sets so we stay around O(3N) instead of O(N^2). Otherwise
  // lookups for duplicate elements will be O(N) and we need them to be O(1).
  std::unordered_set<T*> previous_set = CreateSet(previous_data);
  std::unordered_set<T*> current_set = CreateSet(current_data);

  // TODO: TryFind is just comparing pointers which obviously fails because they point to different memory...

  for (T* current_entry : current_set) {
    // Possibly updated.
    if (T* previous_entry = TryFind(previous_set, current_entry))
      on_found(previous_entry, current_entry);
    // Added
    else
      on_added(current_entry);
  }
  for (T* previous_entry : previous_set) {
    // Removed
    if (!TryFind(current_set, previous_entry))
      on_removed(previous_entry);
}
}
#endif

template<typename T>
void CompareGroups(
  std::vector<T>& previous_data, std::vector<T>& current_data,
  std::function<void(T*)> on_removed, std::function<void(T*)> on_added, std::function<void(T*, T*)> on_found) {
  // TODO: It could be faster to use set_intersection and set_difference to
  //       compute these values. We will have to presort the input by ID, though.

  std::sort(previous_data.begin(), previous_data.end());
  std::sort(current_data.begin(), current_data.end());

  /*
  std::set_difference(
    current_data.begin(), current_data.end(),
    previous_data.begin(), previous_data.end(),
    boost::make_function_output_iterator([](const T& val) {

  }));
  */

  auto prev_it = previous_data.begin();
  auto curr_it = current_data.begin();
  while (prev_it != previous_data.end() && curr_it != current_data.end()) {
    // same id
    if (prev_it->def.id == curr_it->def.id) {
      on_found(&*prev_it, &*curr_it);
      ++prev_it;
      ++curr_it;
    }

    // prev_id is smaller - prev_it has data curr_it does not have.
    else if (prev_it->def.id < curr_it->def.id) {
      on_removed(&*prev_it);
      ++prev_it;
    }

    // prev_id is bigger - curr_it has data prev_it does not have.
    else {
      on_added(&*curr_it);
      ++curr_it;
    }
  }

  // if prev_it still has data, that means it is not in curr_it and was removed.
  while (prev_it != previous_data.end()) {
    on_removed(&*prev_it);
    ++prev_it;
  }

  // if curr_it still has data, that means it is not in prev_it and was added.
  while (curr_it != current_data.end()) {
    on_added(&*curr_it);
    ++curr_it;
  }
}

// TODO: make this const correct.
IndexUpdate ComputeDiff(IdMap* id_map, IndexedFile& previous, IndexedFile& current) {
#define JOIN(a, b) a##b
  // |query_name| is the name of the variable on the query type.
  // |index_name| is the name of the variable on the index type.
  // |type| is the type of the variable.
#define PROCESS_UPDATE_DIFF(query_name, index_name, type) \
  { \
    /* Check for changes. */ \
    std::vector<type> removed, added; \
    bool did_add = ComputeDifferenceForUpdate(JOIN(previous->, index_name), JOIN(current->, index_name), &removed, &added); \
    if (did_add) {\
      std::cout << "Adding mergeable update on " << current->def.short_name << " (" << current->def.usr << ") for field " << #index_name << std::endl; \
      JOIN(update., query_name).push_back(MakeMergeableUpdate(id_map, current->def.id, removed, added)); \
    } \
  }

  assert(previous.usr_to_id == current.usr_to_id);
  assert(previous.file_db == current.file_db);
  IndexUpdate update(id_map);

  // Types
  CompareGroups<IndexedTypeDef>(previous.types, current.types,
    /*onRemoved:*/[&update, &id_map](IndexedTypeDef* def) {
    update.types_removed.push_back(id_map->Remap(def->def.id));
  },
    /*onAdded:*/[&update, &id_map](IndexedTypeDef* def) {
    update.types_added.push_back(QueryableTypeDef(*id_map, *def));
  },
    /*onChanged:*/[&update, &id_map](IndexedTypeDef* previous, IndexedTypeDef* current) {
    if (previous->def != current->def)
      update.types_def_changed.push_back(id_map->Remap(current->def));

    PROCESS_UPDATE_DIFF(types_derived, derived, TypeId);
    PROCESS_UPDATE_DIFF(types_uses, uses, Location);
  });

  // Functions
  CompareGroups<IndexedFuncDef>(previous.funcs, current.funcs,
    /*onRemoved:*/[&update, &id_map](IndexedFuncDef* def) {
    update.funcs_removed.push_back(id_map->Remap(def->def.id));
  },
    /*onAdded:*/[&update, &id_map](IndexedFuncDef* def) {
    update.funcs_added.push_back(QueryableFuncDef(*id_map, *def));
  },
    /*onChanged:*/[&update, &id_map](IndexedFuncDef* previous, IndexedFuncDef* current) {
    if (previous->def != current->def)
      update.funcs_def_changed.push_back(id_map->Remap(current->def));
    PROCESS_UPDATE_DIFF(funcs_declarations, declarations, Location);
    PROCESS_UPDATE_DIFF(funcs_derived, derived, FuncId);
    PROCESS_UPDATE_DIFF(funcs_callers, callers, FuncRef);
    PROCESS_UPDATE_DIFF(funcs_uses, uses, Location);
  });

  // Variables
  CompareGroups<IndexedVarDef>(previous.vars, current.vars,
    /*onRemoved:*/[&update, &id_map](IndexedVarDef* def) {
    update.vars_removed.push_back(id_map->Remap(def->def.id));
  },
    /*onAdded:*/[&update, &id_map](IndexedVarDef* def) {
    update.vars_added.push_back(QueryableVarDef(*id_map, *def));
  },
    /*onChanged:*/[&update, &id_map](IndexedVarDef* previous, IndexedVarDef* current) {
    if (previous->def != current->def)
      update.vars_def_changed.push_back(id_map->Remap(current->def));
    PROCESS_UPDATE_DIFF(vars_uses, uses, Location);
  });

  return update;

#undef PROCESS_UPDATE_DIFF
#undef JOIN
}

// Merge the contents of |source| into |destination|.
void Merge(const IndexUpdate& source, IndexUpdate* destination) {
  // TODO.
}

// Insert the contents of |update| into |db|.
void ApplyIndexUpdate(const IndexUpdate& update, QueryableDatabase* db) {

}



int ma333in(int argc, char** argv) {
  // TODO: Unify UserToIdResolver and FileDb
  UsrToIdResolver usr_to_id(1);
  FileDb file_db(1);

  IndexedFile indexed_file_a = Parse(&usr_to_id, &file_db, "full_tests/index_delta/a_v0.cc", {});
  std::cout << indexed_file_a.ToString() << std::endl;

  std::cout << std::endl;
  IndexedFile indexed_file_b = Parse(&usr_to_id, &file_db, "full_tests/index_delta/a_v1.cc", {});
  std::cout << indexed_file_b.ToString() << std::endl;

  // TODO: We don't need to do ID remapping when computting a diff. Well, we need to do it for the IndexUpdate.
  IdMap dest_ids(2);
  IndexUpdate update = ComputeDiff(&dest_ids, indexed_file_a, indexed_file_b);

  return 0;
}




// TODO: Idea: when indexing and joining to the main db, allow many dbs that
//             are joined to. So that way even if the main db is busy we can
//             still be joining. Joining the partially joined db to the main
//             db should be faster since we will have larger data lanes to use.
// TODO: I think we can run libclang multiple times in one process. So we might
//       only need two processes. Still, for perf reasons it would be good if
//       we could stay in one process. We could probably just use shared
//       memory. May want to run libclang in separate process to protect from
//       crashes/issues there.
// TODO: allow user to store configuration as json? file in home dir; also
//       allow local overrides (scan up dirs)
// TODO: add opt to dump config when starting (--dump-config)
// TODO: allow user to decide some indexer choices, ie, do we mark prototype parameters as usages?
