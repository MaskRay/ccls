#include "query.h"

#include <cstdint>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <iostream>

#include "compilation_database_loader.h"
#include "optional.h"
#include "indexer.h"

//#define CATCH_CONFIG_MAIN
//#include "catch.hpp"

// TODO: Make all copy constructors explicit.









// NOTE: When not inside of a |def| object, there can be duplicates of the same
//       information if that information is contributed from separate sources.
//       If we need to avoid this duplication in the future, we will have to
//       add a refcount.

template<typename In, typename Out>
std::vector<Out> Transform(const std::vector<In>& input, std::function<Out(In)> op) {
  std::vector<Out> result;
  result.reserve(input.size());
  for (const In& in : input)
    result.push_back(op(in));
  return result;
}
Usr MapIdToUsr(IdCache& id_cache, const TypeId& id) {
  return id_cache.type_id_to_usr[id];
}
Usr MapIdToUsr(IdCache& id_cache, const FuncId& id) {
  return id_cache.func_id_to_usr[id];
}
Usr MapIdToUsr(IdCache& id_cache, const VarId& id) {
  return id_cache.var_id_to_usr[id];
}
QueryableLocation MapIdToUsr(IdCache& id_cache, const Location& id) {
  return QueryableLocation(id_cache.file_id_to_file_path[id.file_id()], id.line, id.column, id.interesting);
}

std::vector<Usr> MapIdToUsr(IdCache& id_cache, const std::vector<TypeId>& ids) {
  return Transform<TypeId, Usr>(ids, [&](TypeId id) { return id_cache.type_id_to_usr[id]; });
}
std::vector<Usr> MapIdToUsr(IdCache& id_cache, const std::vector<FuncId>& ids) {
  return Transform<FuncId, Usr>(ids, [&](FuncId id) { return id_cache.func_id_to_usr[id]; });
}
std::vector<Usr> MapIdToUsr(IdCache& id_cache, const std::vector<VarId>& ids) {
  return Transform<VarId, Usr>(ids, [&](VarId id) { return id_cache.var_id_to_usr[id]; });
}
std::vector<UsrRef> MapIdToUsr(IdCache& id_cache, const std::vector<FuncRef>& ids) {
  return Transform<FuncRef, UsrRef>(ids, [&](FuncRef ref) {
    UsrRef result;
    result.loc = MapIdToUsr(id_cache, ref.loc);
    result.usr = id_cache.func_id_to_usr[ref.id];
    return result;
  });
}
std::vector<QueryableLocation> MapIdToUsr(IdCache& id_cache, const std::vector<Location>& ids) {
  return Transform<Location, QueryableLocation>(ids, [&](Location id) {
    return QueryableLocation(id_cache.file_id_to_file_path[id.file_id()], id.line, id.column, id.interesting);
  });
}
QueryableTypeDef::DefUpdate MapIdToUsr(IdCache& id_cache, const TypeDefDefinitionData<>& def) {
  QueryableTypeDef::DefUpdate result(def.usr);
  if (result.definition)
    result.definition = MapIdToUsr(id_cache, def.definition.value());
  if (result.alias_of)
    result.alias_of = MapIdToUsr(id_cache, def.alias_of.value());
  result.parents = MapIdToUsr(id_cache, def.parents);
  result.types = MapIdToUsr(id_cache, def.types);
  result.funcs = MapIdToUsr(id_cache, def.funcs);
  result.vars = MapIdToUsr(id_cache, def.vars);
  return result;
}
QueryableFuncDef::DefUpdate MapIdToUsr(IdCache& id_cache, const FuncDefDefinitionData<>& def) {
  QueryableFuncDef::DefUpdate result(def.usr);
  if (result.definition)
    result.definition = MapIdToUsr(id_cache, def.definition.value());
  if (result.declaring_type)
    result.declaring_type = MapIdToUsr(id_cache, def.declaring_type.value());
  if (result.base)
    result.base = MapIdToUsr(id_cache, def.base.value());
  result.locals = MapIdToUsr(id_cache, def.locals);
  result.callees = MapIdToUsr(id_cache, def.callees);
  return result;
}
QueryableVarDef::DefUpdate MapIdToUsr(IdCache& id_cache, const VarDefDefinitionData<>& def) {
  QueryableVarDef::DefUpdate result(def.usr);
  if (result.declaration)
    result.declaration = MapIdToUsr(id_cache, def.declaration.value());
  if (result.definition)
    result.definition = MapIdToUsr(id_cache, def.definition.value());
  if (result.variable_type)
    result.variable_type = MapIdToUsr(id_cache, def.variable_type.value());
  if (result.declaring_type)
    result.declaring_type = MapIdToUsr(id_cache, def.declaring_type.value());
  return result;
}

QueryableFile::QueryableFile(const IndexedFile& indexed)
  : file_id(indexed.path) {

  auto add_outline = [this, &indexed](Usr usr, Location location) {
    outline.push_back(UsrRef(usr, MapIdToUsr(*indexed.id_cache, location)));
  };

  for (const IndexedTypeDef& def : indexed.types) {
    if (def.def.definition.has_value())
      add_outline(def.def.usr, def.def.definition.value());
  }
  for (const IndexedFuncDef& def : indexed.funcs) {
    for (Location decl : def.declarations)
      add_outline(def.def.usr, decl);
    if (def.def.definition.has_value())
      add_outline(def.def.usr, def.def.definition.value());
  }
  for (const IndexedVarDef& def : indexed.vars) {
    if (def.def.definition.has_value())
      add_outline(def.def.usr, def.def.definition.value());
  }

  std::sort(outline.begin(), outline.end(), [](const UsrRef& a, const UsrRef& b) {
    return a.loc < b.loc;
  });
}

QueryableTypeDef::QueryableTypeDef(IdCache& id_cache, const IndexedTypeDef& indexed)
  : def(MapIdToUsr(id_cache, indexed.def)) {
  derived = MapIdToUsr(id_cache, indexed.derived);
  uses = MapIdToUsr(id_cache, indexed.uses);
}

QueryableFuncDef::QueryableFuncDef(IdCache& id_cache, const IndexedFuncDef& indexed)
  : def(MapIdToUsr(id_cache, indexed.def)) {
  declarations = MapIdToUsr(id_cache, indexed.declarations);
  derived = MapIdToUsr(id_cache, indexed.derived);
  callers = MapIdToUsr(id_cache, indexed.callers);
  uses = MapIdToUsr(id_cache, indexed.uses);
}

QueryableVarDef::QueryableVarDef(IdCache& id_cache, const IndexedVarDef& indexed)
  : def(MapIdToUsr(id_cache, indexed.def)) {
  uses = MapIdToUsr(id_cache, indexed.uses);
}









// TODO: For space reasons, it may make sense to map Usr -> offset inside of global storage. But not for intermediate or disk-storage.
//       We can probably eliminate most of that pain by coming up with our own UsrDb concept which interns the Usr strings. We can make
//       the pain of a global UsrDb less by
//          (parallel)clangindex -> (main)commit USRs to global -> (parallel)transfer IDs to global USRs -> (main)import

// TODO: remove GroupId concept.

struct CachedIndexedFile {
  // Path to the file indexed.
  std::string path;

  // TODO: Make sure that |previous_index| and |current_index| use the same id
  // to USR mapping. This lets us greatly speed up difference computation.

  // The previous index. This is used for index updates, so we only apply a
  // an update diff when changing the global db.
  optional<IndexedFile> previous_index;
  IndexedFile current_index;

  CachedIndexedFile(const IndexedFile& indexed) : current_index(indexed) {}
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
    if (prev_it->def.usr == curr_it->def.usr) {
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
    else if (prev_it->def.usr < curr_it->def.usr) {
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

















IndexUpdate::IndexUpdate(IndexedFile& file) {
  files_added.push_back(QueryableFile(file));
  for (const IndexedTypeDef& def : file.types)
    types_added.push_back(QueryableTypeDef(*file.id_cache, def));
  for (const IndexedFuncDef& def : file.funcs)
    funcs_added.push_back(QueryableFuncDef(*file.id_cache, def));
  for (const IndexedVarDef& def : file.vars)
    vars_added.push_back(QueryableVarDef(*file.id_cache, def));
}

IndexUpdate::IndexUpdate(IndexedFile& previous_file, IndexedFile& current_file) {
#define JOIN(a, b) a##b
  // |query_name| is the name of the variable on the query type.
  // |index_name| is the name of the variable on the index type.
  // |type| is the type of the variable.
#define PROCESS_UPDATE_DIFF(query_name, index_name, type) \
  { \
    /* Check for changes. */ \
    std::vector<type> removed, added; \
    bool did_add = ComputeDifferenceForUpdate( \
                      MapIdToUsr(*previous_file.id_cache, JOIN(previous_def->, index_name)), \
                      MapIdToUsr(*current_file.id_cache, JOIN(current_def->, index_name)), \
                      &removed, &added); \
    if (did_add) {\
      std::cout << "Adding mergeable update on " << current_def->def.short_name << " (" << current_def->def.usr << ") for field " << #index_name << std::endl; \
      query_name.push_back(MergeableUpdate<type>(current_def->def.usr, removed, added)); \
    } \
  }


  // File
  do {
    // Outline is a special property and needs special handling, because it is a computed property
    // of the IndexedFile (ie, to view it we need to construct a QueryableFile instance).
    assert(previous_file.path == current_file.path);
    QueryableFile previous_queryable_file(previous_file);
    QueryableFile current_queryable_file(previous_file);
    std::vector<UsrRef> removed, added;
    bool did_add = ComputeDifferenceForUpdate(
      previous_queryable_file.outline,
      current_queryable_file.outline,
      &removed, &added);
    if (did_add) {
      std::cout << "Adding mergeable update on outline (" << current_file.path << ")" << std::endl;
      files_outline.push_back(MergeableUpdate<UsrRef>(current_file.path, removed, added));
    }
  } while (false); // do while false instead of just {} to appease Visual Studio code formatter.

  // Types
  CompareGroups<IndexedTypeDef>(previous_file.types, current_file.types,
    /*onRemoved:*/[this](IndexedTypeDef* def) {
    types_removed.push_back(def->def.usr);
  },
    /*onAdded:*/[this, &current_file](IndexedTypeDef* def) {
    types_added.push_back(QueryableTypeDef(*current_file.id_cache, *def));
  },
    /*onFound:*/[this, &previous_file, &current_file](IndexedTypeDef* previous_def, IndexedTypeDef* current_def) {
    QueryableTypeDef::DefUpdate previous_remapped_def = MapIdToUsr(*previous_file.id_cache, previous_def->def);
    QueryableTypeDef::DefUpdate current_remapped_def = MapIdToUsr(*current_file.id_cache, current_def->def);
    if (previous_remapped_def != current_remapped_def)
      types_def_changed.push_back(current_remapped_def);

    PROCESS_UPDATE_DIFF(types_derived, derived, Usr);
    PROCESS_UPDATE_DIFF(types_uses, uses, QueryableLocation);
  });

  // Functions
  CompareGroups<IndexedFuncDef>(previous_file.funcs, current_file.funcs,
    /*onRemoved:*/[this](IndexedFuncDef* def) {
    funcs_removed.push_back(def->def.usr);
  },
    /*onAdded:*/[this, &current_file](IndexedFuncDef* def) {
    funcs_added.push_back(QueryableFuncDef(*current_file.id_cache, *def));
  },
    /*onFound:*/[this, &previous_file, &current_file](IndexedFuncDef* previous_def, IndexedFuncDef* current_def) {
    QueryableFuncDef::DefUpdate previous_remapped_def = MapIdToUsr(*previous_file.id_cache, previous_def->def);
    QueryableFuncDef::DefUpdate current_remapped_def = MapIdToUsr(*current_file.id_cache, current_def->def);
    if (previous_remapped_def != current_remapped_def)
      funcs_def_changed.push_back(current_remapped_def);

    PROCESS_UPDATE_DIFF(funcs_declarations, declarations, QueryableLocation);
    PROCESS_UPDATE_DIFF(funcs_derived, derived, Usr);
    PROCESS_UPDATE_DIFF(funcs_callers, callers, UsrRef);
    PROCESS_UPDATE_DIFF(funcs_uses, uses, QueryableLocation);
  });

  // Variables
  CompareGroups<IndexedVarDef>(previous_file.vars, current_file.vars,
    /*onRemoved:*/[this](IndexedVarDef* def) {
    vars_removed.push_back(def->def.usr);
  },
    /*onAdded:*/[this, &current_file](IndexedVarDef* def) {
    vars_added.push_back(QueryableVarDef(*current_file.id_cache, *def));
  },
    /*onFound:*/[this, &previous_file, &current_file](IndexedVarDef* previous_def, IndexedVarDef* current_def) {
    QueryableVarDef::DefUpdate previous_remapped_def = MapIdToUsr(*previous_file.id_cache, previous_def->def);
    QueryableVarDef::DefUpdate current_remapped_def = MapIdToUsr(*current_file.id_cache, current_def->def);
    if (previous_remapped_def != current_remapped_def)
      vars_def_changed.push_back(current_remapped_def);

    PROCESS_UPDATE_DIFF(vars_uses, uses, QueryableLocation);
  });

#undef PROCESS_UPDATE_DIFF
#undef JOIN
}

void IndexUpdate::Merge(const IndexUpdate& update) {
#define INDEX_UPDATE_MERGE(name) \
    AddRange(&name, update.##name);

  INDEX_UPDATE_MERGE(files_removed);
  INDEX_UPDATE_MERGE(files_added);
  INDEX_UPDATE_MERGE(files_outline);

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


























void QueryableDatabase::RemoveUsrs(const std::vector<Usr>& to_remove) {
  // TODO: Removing usrs is tricky because it means we will have to rebuild idx locations. I'm thinking we just nullify
  //       the entry instead of actually removing the data. The index could be massive.
  for (Usr usr : to_remove)
    usr_to_symbol[usr].kind = SymbolKind::Invalid;
}

void QueryableDatabase::Import(const std::vector<QueryableFile>& defs) {
  for (auto& def : defs) {
    usr_to_symbol[def.file_id] = SymbolIdx(SymbolKind::File, files.size());
    files.push_back(def);
  }
}

void QueryableDatabase::Import(const std::vector<QueryableTypeDef>& defs) {
  for (auto& def : defs) {
    usr_to_symbol[def.def.usr] = SymbolIdx(SymbolKind::Type, types.size());
    types.push_back(def);
  }
}

void QueryableDatabase::Import(const std::vector<QueryableFuncDef>& defs) {
  for (auto& def : defs) {
    usr_to_symbol[def.def.usr] = SymbolIdx(SymbolKind::Func, funcs.size());
    funcs.push_back(def);
  }
}

void QueryableDatabase::Import(const std::vector<QueryableVarDef>& defs) {
  for (auto& def : defs) {
    usr_to_symbol[def.def.usr] = SymbolIdx(SymbolKind::Var, vars.size());
    vars.push_back(def);
  }
}

void QueryableDatabase::Update(const std::vector<QueryableTypeDef::DefUpdate>& updates) {
  for (auto& def : updates) {
    SymbolIdx idx = usr_to_symbol[def.usr];
    assert(idx.kind == SymbolKind::Type);
    types[idx.idx].def = def;
  }
}

void QueryableDatabase::Update(const std::vector<QueryableFuncDef::DefUpdate>& updates) {
  for (auto& def : updates) {
    SymbolIdx idx = usr_to_symbol[def.usr];
    assert(idx.kind == SymbolKind::Func);
    funcs[idx.idx].def = def;
  }
}

void QueryableDatabase::Update(const std::vector<QueryableVarDef::DefUpdate>& updates) {
  for (auto& def : updates) {
    SymbolIdx idx = usr_to_symbol[def.usr];
    assert(idx.kind == SymbolKind::Var);
    vars[idx.idx].def = def;
  }
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

void QueryableDatabase::ApplyIndexUpdate(IndexUpdate* update) {
#define JOIN(a, b) a##b
#define HANDLE_MERGEABLE(update_var_name, def_var_name, storage_name) \
  for (auto merge_update : JOIN(update->, update_var_name)) { \
    SymbolIdx index = usr_to_symbol[merge_update.usr]; \
    auto* def = &JOIN(storage_name, [index.idx]); \
    AddRange(JOIN(&def->, def_var_name), merge_update.to_add); \
    RemoveRange(JOIN(&def->, def_var_name), merge_update.to_remove); \
  }

  RemoveUsrs(update->files_removed);
  Import(update->files_added);
  HANDLE_MERGEABLE(files_outline, outline, files);

  RemoveUsrs(update->types_removed);
  Import(update->types_added);
  Update(update->types_def_changed);
  HANDLE_MERGEABLE(types_derived, derived, types);
  HANDLE_MERGEABLE(types_uses, uses, types);

  RemoveUsrs(update->funcs_removed);
  Import(update->funcs_added);
  Update(update->funcs_def_changed);
  HANDLE_MERGEABLE(funcs_declarations, declarations, funcs);
  HANDLE_MERGEABLE(funcs_derived, derived, funcs);
  HANDLE_MERGEABLE(funcs_callers, callers, funcs);
  HANDLE_MERGEABLE(funcs_uses, uses, funcs);

  RemoveUsrs(update->vars_removed);
  Import(update->vars_added);
  Update(update->vars_def_changed);
  HANDLE_MERGEABLE(vars_uses, uses, vars);

#undef HANDLE_MERGEABLE
#undef JOIN
}










int main233(int argc, char** argv) {
  IdCache id_cache;

  IndexedFile indexed_file_a = Parse(&id_cache, "full_tests/index_delta/a_v0.cc", {});
  std::cout << indexed_file_a.ToString() << std::endl;

  std::cout << std::endl;
  IndexedFile indexed_file_b = Parse(&id_cache, "full_tests/index_delta/a_v1.cc", {});
  std::cout << indexed_file_b.ToString() << std::endl;
  // TODO: We don't need to do ID remapping when computting a diff. Well, we need to do it for the IndexUpdate.
  IndexUpdate import(indexed_file_a);
  /*
  dest_ids.Import(indexed_file_b.file_db, indexed_file_b.id_cache);
  IndexUpdate update = ComputeDiff(indexed_file_a, indexed_file_b);
  */
  QueryableDatabase db;
  db.ApplyIndexUpdate(&import);
  //db.ApplyIndexUpdate(&update);

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
