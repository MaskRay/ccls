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
  // TODO: id resolution is broken. We need to resolve same fundamental USR to same ID. Problem is that multiple USRs
  //       can have different source IDs.

  // The first vector is indexed by TId::group.
  // The second vector is indexed by TId::id.
  template<typename TId>
  using GroupMap = std::vector<std::unordered_map<TId, TId>>;

  template<typename TId>
  using GroupToUsrMap = std::vector<std::unordered_map<TId, std::string>>;

  GroupToUsrMap<FileId> group_file_id_to_usr;
  GroupToUsrMap<TypeId> group_type_id_to_usr;
  GroupToUsrMap<FuncId> group_func_id_to_usr;
  GroupToUsrMap<VarId> group_var_id_to_usr;
  std::unordered_map<std::string, FileId> usr_to_file_id;
  std::unordered_map<std::string, TypeId> usr_to_type_id;
  std::unordered_map<std::string, FuncId> usr_to_func_id;
  std::unordered_map<std::string, VarId> usr_to_var_id;

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

  void Import(IdMap* other_id_map) {
    // TODO: Implement me
    // TODO: Let's refactor the entire ID management system. DB should
    //       lose concept of Var/Type/etc id and just use USR, since we
    //       are not going to be storing indices into arrays.
  }

  void Import(FileDb* file_db, UsrToIdResolver* usr_to_id) {
    int group = usr_to_id->group;

    if (group >= group_file_id_to_usr.size()) {
      group_file_id_to_usr.resize(group + 1);
      group_type_id_to_usr.resize(group + 1);
      group_func_id_to_usr.resize(group + 1);
      group_var_id_to_usr.resize(group + 1);
    }

    group_file_id_to_usr[group] = file_db->file_id_to_file_path;

    std::unordered_map<TypeId, std::string>& type_id_to_usr = group_type_id_to_usr[group];
    for (auto& entry : usr_to_id->usr_to_type_id)
      type_id_to_usr[entry.second] = entry.first;
    std::unordered_map<FuncId, std::string>& func_id_to_usr = group_func_id_to_usr[group];
    for (auto& entry : usr_to_id->usr_to_func_id)
      func_id_to_usr[entry.second] = entry.first;
    std::unordered_map<VarId, std::string>& var_id_to_usr = group_var_id_to_usr[group];
    for (auto& entry : usr_to_id->usr_to_var_id)
      var_id_to_usr[entry.second] = entry.first;
  }

  template<typename TId>
  inline TId GenericRemap(
    GroupMap<TId>* map,
    GroupToUsrMap<TId>* group_id_to_usr, std::unordered_map<std::string, TId>* usr_to_id,
    int64_t* next_id, TId from) {

    if (from.group == target_group)
      return from;

    // Find the group that |from| belongs to. Create groups if needed.
    if (from.group >= map->size())
      map->resize(from.group + 1);

    // If the group doesn't have an ID already mapped out for |from|, map it.
    std::unordered_map<TId, TId> group = (*map)[from.group];

    // Lookup the id from the group or add it.
    auto it = group.find(from);
    if (it == group.end()) {
      // NOTE: If the following asserts make sure that from.group is registered inside
      //       of the group_*_id_to_usr variables.

      // Before adding a new id, we need to check if we have already added it.
      const std::string& usr_for_id = (*group_id_to_usr)[from.group][from];
      auto it = usr_to_id->find(usr_for_id);
      if (it != usr_to_id->end()) {
        return it->second;
      }
      // This is a brand new id we haven't seen before.
      else {
        TId result(target_group, (*next_id)++);
        group[from] = result;
        (*usr_to_id)[usr_for_id] = result;
        return result;
      }
    }
    return it->second;
  }

  template<typename TId>
  inline std::vector<TId> GenericVectorRemap(GroupMap<TId>* map, int64_t* next_id, const std::vector<TId>& from) {
    std::vector<TId> result;
    result.reserve(from.size());
    for (const TId& e : from)
      result.push_back(Remap(e));
    return result;
    /*
    if (from.empty())
      return {};

    int group_id = from[0].group;
    if (group_id >= map->size())
      map->resize(group_id + 1);

    std::unordered_map<TId, TId> group = (*map)[group_id];

    std::vector<TId> result;
    result.reserve(from.size());
    for (TId id : from) {
      if (id.group == target_group) {
        result.push_back(id);
        continue;
      }

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
    */
  }

  FileId Remap(FileId from) {
    return GenericRemap<FileId>(&remap_file_id, &group_file_id_to_usr, &usr_to_file_id, &next_file_id, from);
  }
  TypeId Remap(TypeId from) {
    return GenericRemap(&remap_type_id, &group_type_id_to_usr, &usr_to_type_id, &next_type_id, from);
  }
  FuncId Remap(FuncId from) {
    return GenericRemap(&remap_func_id, &group_func_id_to_usr, &usr_to_func_id, &next_func_id, from);
  }
  VarId Remap(VarId from) {
    return GenericRemap(&remap_var_id, &group_var_id_to_usr, &usr_to_var_id, &next_var_id, from);
  }
  Location Remap(Location from) {
    FileId file = Remap(from.file_id());
    from.raw_file_group = file.group;
    from.raw_file_id = file.id;
    return from;
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
  QueryableTypeDef Remap(QueryableTypeDef def) {
    def.def = Remap(def.def);
    def.derived = Remap(def.derived);
    def.uses = Remap(def.uses);
    return def;
  }
  QueryableFuncDef Remap(QueryableFuncDef def) {
    def.def = Remap(def.def);
    def.declarations = Remap(def.declarations);
    def.derived = Remap(def.derived);
    def.callers = Remap(def.callers);
    def.uses = Remap(def.uses);
    return def;
  }
  QueryableVarDef Remap(QueryableVarDef def) {
    def.def = Remap(def.def);
    def.uses = Remap(def.uses);
    return def;
  }
  template<typename TId, typename TValue>
  MergeableUpdate<TId, TValue> Remap(MergeableUpdate<TId, TValue> update) {
    update.id = Remap(update.id);
    update.to_add = Remap(update.to_add);
    update.to_remove = Remap(update.to_remove);
    return update;
  }


  template<typename T>
  std::vector<T> Remap(const std::vector<T>& from) {
    std::vector<T> result;
    result.reserve(from.size());
    for (const T& e : from)
      result.push_back(Remap(e));
    return result;
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


QueryableTypeDef::QueryableTypeDef(IdMap& id_map, const IndexedTypeDef& indexed)
  : def(id_map.Remap(indexed.def)) {
  derived = id_map.Remap(indexed.derived);
  uses = id_map.Remap(indexed.uses);
}

QueryableFuncDef::QueryableFuncDef(IdMap& id_map, const IndexedFuncDef& indexed)
  : def(id_map.Remap(indexed.def)) {
  declarations = id_map.Remap(indexed.declarations);
  derived = id_map.Remap(indexed.derived);
  callers = id_map.Remap(indexed.callers);
  uses = id_map.Remap(indexed.uses);
}

QueryableVarDef::QueryableVarDef(IdMap& id_map, const IndexedVarDef& indexed)
  : def(id_map.Remap(indexed.def)) {
  uses = id_map.Remap(indexed.uses);
}

struct QueryableEntry {
  const char* const str;
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

template<typename T>
void AddRange(std::vector<T>* dest, const std::vector<T>& to_add) {
  for (const T& e : to_add)
    dest->push_back(e);
}

template<typename T>
void RemoveRange(std::vector<T>* dest, const std::vector<T>& to_remove) {
  auto it = std::remove_if(dest->begin(), dest->end(), [&](const T& t) {
    // TODO: make to_remove a set?
    return std::find(to_remove.begin(), to_remove.end(), t) != to_remove.end();
  });
  if (it != dest->end())
    dest->erase(it);
}

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
  IndexUpdate(IdMap* id_map, IndexedFile& file);

  void Remap(IdMap* map) {
    id_map = map;

#define INDEX_UPDATE_REMAP(name) \
    name = id_map->Remap(name);

    INDEX_UPDATE_REMAP(types_removed);
    INDEX_UPDATE_REMAP(types_added);
    INDEX_UPDATE_REMAP(types_def_changed);
    INDEX_UPDATE_REMAP(types_derived);
    INDEX_UPDATE_REMAP(types_uses);

    INDEX_UPDATE_REMAP(funcs_removed);
    INDEX_UPDATE_REMAP(funcs_added);
    INDEX_UPDATE_REMAP(funcs_def_changed);
    INDEX_UPDATE_REMAP(funcs_declarations);
    INDEX_UPDATE_REMAP(funcs_derived);
    INDEX_UPDATE_REMAP(funcs_callers);
    INDEX_UPDATE_REMAP(funcs_uses);

    INDEX_UPDATE_REMAP(vars_removed);
    INDEX_UPDATE_REMAP(vars_added);
    INDEX_UPDATE_REMAP(vars_def_changed);
    INDEX_UPDATE_REMAP(vars_uses);

#undef INDEX_UPDATE_REMAP
  }

  // Merges the contents of |update| into this IndexUpdate instance.
  void Merge(const IndexUpdate& update) {
#define INDEX_UPDATE_MERGE(name) \
    AddRange(&name, id_map->Remap(update.##name));

    INDEX_UPDATE_MERGE(types_removed);
    INDEX_UPDATE_MERGE(types_added);
    INDEX_UPDATE_MERGE(types_def_changed);
    INDEX_UPDATE_MERGE(types_derived);
    INDEX_UPDATE_MERGE(types_uses);

    INDEX_UPDATE_MERGE(funcs_removed);
    INDEX_UPDATE_MERGE(funcs_added);
    INDEX_UPDATE_MERGE(funcs_def_changed);
    INDEX_UPDATE_MERGE(funcs_declarations);
    INDEX_UPDATE_MERGE(funcs_derived);
    INDEX_UPDATE_MERGE(funcs_callers);
    INDEX_UPDATE_MERGE(funcs_uses);

    INDEX_UPDATE_MERGE(vars_removed);
    INDEX_UPDATE_MERGE(vars_added);
    INDEX_UPDATE_MERGE(vars_def_changed);
    INDEX_UPDATE_MERGE(vars_uses);

#undef INDEX_UPDATE_MERGE
  }
};

IndexUpdate::IndexUpdate(IdMap* id_map, IndexedFile& file) : id_map(id_map) {
  id_map->Import(file.file_db, file.usr_to_id);

  for (IndexedTypeDef& def : file.types)
    types_added.push_back(QueryableTypeDef(*id_map, def));
  for (IndexedFuncDef& def : file.funcs)
    funcs_added.push_back(QueryableFuncDef(*id_map, def));
  for (IndexedVarDef& def : file.vars)
    vars_added.push_back(QueryableVarDef(*id_map, def));
}


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

template<typename T>
void CompareGroups(
  std::vector<T>& previous_data, std::vector<T>& current_data,
  std::function<void(T*)> on_removed, std::function<void(T*)> on_added, std::function<void(T*, T*)> on_found) {

  std::sort(previous_data.begin(), previous_data.end());
  std::sort(current_data.begin(), current_data.end());

  auto prev_it = previous_data.begin();
  auto curr_it = current_data.begin();
  while (prev_it != previous_data.end() && curr_it != current_data.end()) {
    // same id
    if (prev_it->def.id == curr_it->def.id) {
      if (!prev_it->is_bad_def && !curr_it->is_bad_def)
        on_found(&*prev_it, &*curr_it);
      else if (prev_it->is_bad_def)
        on_added(&*curr_it);
      else if (curr_it->is_bad_def)
        on_removed(&*curr_it);

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

  // TypeId to index in |types| (same for funcs, vars)
  std::unordered_map<TypeId, int> type_id_to_index;
  std::unordered_map<FuncId, int> func_id_to_index;
  std::unordered_map<VarId, int> var_id_to_index;

  // |files| is indexed by FileId. Retrieve a FileId from a path using
  // |file_db|.
  FileDb file_db;
  std::vector<QueryableFile> files;

  // When importing data into the global db we need to remap ids from an
  // arbitrary group into the global group.
  IdMap id_map;

  QueryableDatabase(GroupId group);

  // Insert the contents of |update| into |db|.
  void ApplyIndexUpdate(IndexUpdate* update);
};

template<typename TDef, typename TId>
void RemoveAll(std::unordered_map<TId, int>* id_map, std::vector<TDef>* defs, const std::vector<TId>& ids_to_remove) {
  auto to_erase = std::remove_if(defs->begin(), defs->end(), [&](const TDef& def) {
    // TODO: make ids_to_remove a set?
    return std::find(ids_to_remove.begin(), ids_to_remove.end(), def.def.id) != ids_to_remove.end();
  });

  for (auto it = to_erase; it != defs->end(); ++it) {
    id_map->erase(it->def.id);
  }

  defs->erase(to_erase, defs->end());
}

template<typename TDef, typename TId>
void AddAll(std::unordered_map<TId, int>* id_map, std::vector<TDef>* defs, const std::vector<TDef>& to_add) {
  for (const TDef& def : to_add) {
    (*id_map)[def.def.id] = defs->size();
    defs->push_back(def);
  }
}

template<typename TDef, typename TId>
void ApplyUpdates(std::unordered_map<TId, int>* id_map, std::vector<TDef>* defs, const std::vector<typename TDef::DefUpdate>& updates) {
  for (const typename TDef::DefUpdate& def : updates) {
    TId id = def.id;
    int index = (*id_map)[id];
    (*defs)[index].def = def;
  }
}

QueryableDatabase::QueryableDatabase(GroupId group) : id_map(group), file_db(group) {}

void QueryableDatabase::ApplyIndexUpdate(IndexUpdate* update) {
  id_map.Import(update->id_map);

#define JOIN(a, b) a##b
#define HANDLE_MERGEABLE(update_var_name, def_var_name, index_name, storage_name) \
  for (auto merge_update : JOIN(update->, update_var_name)) { \
    int index = JOIN(index_name, [merge_update.id]); \
    auto* def = &JOIN(storage_name, [index]); \
    AddRange(JOIN(&def->, def_var_name), merge_update.to_add); \
    RemoveRange(JOIN(&def->, def_var_name), merge_update.to_remove); \
  }

  update->Remap(&id_map);

  RemoveAll(&type_id_to_index, &types, update->types_removed);
  AddAll(&type_id_to_index, &types, update->types_added);
  ApplyUpdates(&type_id_to_index, &types, update->types_def_changed);
  HANDLE_MERGEABLE(types_derived, derived, type_id_to_index, types);
  HANDLE_MERGEABLE(types_uses, uses, type_id_to_index, types);

  RemoveAll(&func_id_to_index, &funcs, update->funcs_removed);
  AddAll(&func_id_to_index, &funcs, update->funcs_added);
  ApplyUpdates(&func_id_to_index, &funcs, update->funcs_def_changed);
  HANDLE_MERGEABLE(funcs_declarations, declarations, func_id_to_index, funcs);
  HANDLE_MERGEABLE(funcs_derived, derived, func_id_to_index, funcs);
  HANDLE_MERGEABLE(funcs_callers, callers, func_id_to_index, funcs);
  HANDLE_MERGEABLE(funcs_uses, uses, func_id_to_index, funcs);

  RemoveAll(&var_id_to_index, &vars, update->vars_removed);
  AddAll(&var_id_to_index, &vars, update->vars_added);
  ApplyUpdates(&var_id_to_index, &vars, update->vars_def_changed);
  HANDLE_MERGEABLE(vars_uses, uses, var_id_to_index, vars);

#undef HANDLE_MERGEABLE
#undef JOIN
}










int main(int argc, char** argv) {
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
  IndexUpdate import(&dest_ids, indexed_file_a);
  dest_ids.Import(indexed_file_b.file_db, indexed_file_b.usr_to_id);
  IndexUpdate update = ComputeDiff(&dest_ids, indexed_file_a, indexed_file_b);

  QueryableDatabase db(5);
  db.ApplyIndexUpdate(&import);
  db.ApplyIndexUpdate(&update);

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
