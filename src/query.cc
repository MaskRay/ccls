#include "query.h"

#include "indexer.h"

#include <optional.h>

#include <cstdint>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <iostream>

// TODO: Make all copy constructors explicit.





template<typename In, typename Out>
std::vector<Out> Transform(const std::vector<In>& input, std::function<Out(In)> op) {
  std::vector<Out> result;
  result.reserve(input.size());
  for (const In& in : input)
    result.push_back(op(in));
  return result;
}

// TODO: These functions are failing. Investigate why.
Usr MapIdToUsr(const IdCache& id_cache, const TypeId& id) {
  assert(id_cache.type_id_to_usr.find(id) != id_cache.type_id_to_usr.end());
  return id_cache.type_id_to_usr.find(id)->second;
}
Usr MapIdToUsr(const IdCache& id_cache, const FuncId& id) {
  assert(id_cache.func_id_to_usr.find(id) != id_cache.func_id_to_usr.end());
  return id_cache.func_id_to_usr.find(id)->second;
}
Usr MapIdToUsr(const IdCache& id_cache, const VarId& id) {
  assert(id_cache.var_id_to_usr.find(id) != id_cache.var_id_to_usr.end());
  return id_cache.var_id_to_usr.find(id)->second;
}
QueryableLocation MapIdToUsr(const IdCache& id_cache, const Range& range) {
  return QueryableLocation(id_cache.primary_file, range);
}

std::vector<Usr> MapIdToUsr(const IdCache& id_cache, const std::vector<TypeId>& ids) {
  return Transform<TypeId, Usr>(ids, [&](TypeId id) {
    assert(id_cache.type_id_to_usr.find(id) != id_cache.type_id_to_usr.end());
    return id_cache.type_id_to_usr.find(id)->second;
  });
}
std::vector<Usr> MapIdToUsr(const IdCache& id_cache, const std::vector<FuncId>& ids) {
  return Transform<FuncId, Usr>(ids, [&](FuncId id) {
    assert(id_cache.func_id_to_usr.find(id) != id_cache.func_id_to_usr.end());
    return id_cache.func_id_to_usr.find(id)->second;
  });
}
std::vector<Usr> MapIdToUsr(const IdCache& id_cache, const std::vector<VarId>& ids) {
  return Transform<VarId, Usr>(ids, [&](VarId id) {
    assert(id_cache.var_id_to_usr.find(id) != id_cache.var_id_to_usr.end());
    return id_cache.var_id_to_usr.find(id)->second;
  });
}
std::vector<UsrRef> MapIdToUsr(const IdCache& id_cache, const std::vector<FuncRef>& ids) {
  return Transform<FuncRef, UsrRef>(ids, [&](FuncRef ref) {
    assert(id_cache.func_id_to_usr.find(ref.id) != id_cache.func_id_to_usr.end());

    UsrRef result;
    result.loc = MapIdToUsr(id_cache, ref.loc);
    result.usr = id_cache.func_id_to_usr.find(ref.id)->second;
    return result;
  });
}
std::vector<QueryableLocation> MapIdToUsr(const IdCache& id_cache, const std::vector<Range>& ids) {
  return Transform<Range, QueryableLocation>(ids, [&](Range range) {
    return QueryableLocation(id_cache.primary_file, range);
  });
}
QueryableTypeDef::DefUpdate MapIdToUsr(const IdCache& id_cache, const IndexedTypeDef::Def& def) {
  QueryableTypeDef::DefUpdate result(def.usr);
  result.short_name = def.short_name;
  result.qualified_name = def.qualified_name;
  if (def.definition_spelling)
    result.definition_spelling = MapIdToUsr(id_cache, def.definition_spelling.value());
  if (def.definition_extent)
    result.definition_extent = MapIdToUsr(id_cache, def.definition_extent.value());
  if (def.alias_of)
    result.alias_of = MapIdToUsr(id_cache, def.alias_of.value());
  result.parents = MapIdToUsr(id_cache, def.parents);
  result.types = MapIdToUsr(id_cache, def.types);
  result.funcs = MapIdToUsr(id_cache, def.funcs);
  result.vars = MapIdToUsr(id_cache, def.vars);
  return result;
}
QueryableFuncDef::DefUpdate MapIdToUsr(const IdCache& id_cache, const IndexedFuncDef::Def& def) {
  QueryableFuncDef::DefUpdate result(def.usr);
  result.short_name = def.short_name;
  result.qualified_name = def.qualified_name;
  if (def.definition_spelling)
    result.definition_spelling = MapIdToUsr(id_cache, def.definition_spelling.value());
  if (def.definition_extent)
    result.definition_extent = MapIdToUsr(id_cache, def.definition_extent.value());
  if (def.declaring_type)
    result.declaring_type = MapIdToUsr(id_cache, def.declaring_type.value());
  if (def.base)
    result.base = MapIdToUsr(id_cache, def.base.value());
  result.locals = MapIdToUsr(id_cache, def.locals);
  result.callees = MapIdToUsr(id_cache, def.callees);
  return result;
}
QueryableVarDef::DefUpdate MapIdToUsr(const IdCache& id_cache, const IndexedVarDef::Def& def) {
  QueryableVarDef::DefUpdate result(def.usr);
  result.short_name = def.short_name;
  result.qualified_name = def.qualified_name;
  if (def.declaration)
    result.declaration = MapIdToUsr(id_cache, def.declaration.value());
  if (def.definition_spelling)
    result.definition_spelling = MapIdToUsr(id_cache, def.definition_spelling.value());
  if (def.definition_extent)
    result.definition_extent = MapIdToUsr(id_cache, def.definition_extent.value());
  if (def.variable_type)
    result.variable_type = MapIdToUsr(id_cache, def.variable_type.value());
  if (def.declaring_type)
    result.declaring_type = MapIdToUsr(id_cache, def.declaring_type.value());
  return result;
}

QueryableFile::Def BuildFileDef(const IndexedFile& indexed) {
  QueryableFile::Def def;
  def.usr = indexed.path;

  auto add_outline = [&def, &indexed](Usr usr, Range range) {
    def.outline.push_back(UsrRef(usr, MapIdToUsr(indexed.id_cache, range)));
  };
  auto add_all_symbols = [&def, &indexed](Usr usr, Range range) {
    def.all_symbols.push_back(UsrRef(usr, MapIdToUsr(indexed.id_cache, range)));
  };

  for (const IndexedTypeDef& def : indexed.types) {
    if (def.def.definition_spelling.has_value())
      add_all_symbols(def.def.usr, def.def.definition_spelling.value());
    if (def.def.definition_extent.has_value())
      add_outline(def.def.usr, def.def.definition_extent.value());
    for (const Range& use : def.uses)
      add_all_symbols(def.def.usr, use);
  }
  for (const IndexedFuncDef& def : indexed.funcs) {
    if (def.def.definition_spelling.has_value())
      add_all_symbols(def.def.usr, def.def.definition_spelling.value());
    if (def.def.definition_extent.has_value())
      add_outline(def.def.usr, def.def.definition_extent.value());
    for (Range decl : def.declarations) {
      add_outline(def.def.usr, decl);
      add_all_symbols(def.def.usr, decl);
    }
    for (const Range& use : def.uses)
      add_all_symbols(def.def.usr, use);
  }
  for (const IndexedVarDef& def : indexed.vars) {
    if (def.def.definition_spelling.has_value())
      add_all_symbols(def.def.usr, def.def.definition_spelling.value());
    if (def.def.definition_extent.has_value())
      add_outline(def.def.usr, def.def.definition_extent.value());
    for (const Range& use : def.uses)
      add_all_symbols(def.def.usr, use);
  }

  std::sort(def.outline.begin(), def.outline.end(), [](const UsrRef& a, const UsrRef& b) {
    return a.loc.range.start < b.loc.range.start;
  });
  std::sort(def.all_symbols.begin(), def.all_symbols.end(), [](const UsrRef& a, const UsrRef& b) {
    return a.loc.range.start < b.loc.range.start;
  });

  return def;
}

QueryableFile::QueryableFile(const IndexedFile& indexed)
  : def(BuildFileDef(indexed)) {}

QueryableTypeDef::QueryableTypeDef(IdCache& id_cache, const IndexedTypeDef& indexed)
  : def(MapIdToUsr(id_cache, indexed.def)) {
  derived = MapIdToUsr(id_cache, indexed.derived);
  instantiations = MapIdToUsr(id_cache, indexed.instantiations);
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
      //if (!prev_it->is_bad_def && !curr_it->is_bad_def)
      on_found(&*prev_it, &*curr_it);
      //else if (prev_it->is_bad_def)
      //  on_added(&*curr_it);
      //else if (curr_it->is_bad_def)
      //  on_removed(&*curr_it);

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
















// static
IndexUpdate IndexUpdate::CreateImport(IndexedFile& file) {
  // Return standard diff constructor but with an empty file so everything is
  // added.
  IndexedFile previous(file.path);
  return IndexUpdate(previous, file);
}

// static
IndexUpdate IndexUpdate::CreateDelta(IndexedFile& current, IndexedFile& updated) {
  return IndexUpdate(current, updated);
}

IndexUpdate::IndexUpdate(IndexedFile& previous_file, IndexedFile& current_file) {
  // |query_name| is the name of the variable on the query type.
  // |index_name| is the name of the variable on the index type.
  // |type| is the type of the variable.
#define PROCESS_UPDATE_DIFF(query_name, index_name, type) \
  { \
    /* Check for changes. */ \
    std::vector<type> removed, added; \
    auto previous = MapIdToUsr(previous_file.id_cache, previous_def->index_name); \
    auto current = MapIdToUsr(current_file.id_cache, current_def->index_name); \
    bool did_add = ComputeDifferenceForUpdate( \
                      previous, current, \
                      &removed, &added); \
    if (did_add) {\
      std::cerr << "Adding mergeable update on " << current_def->def.short_name << " (" << current_def->def.usr << ") for field " << #index_name << std::endl; \
      query_name.push_back(MergeableUpdate<type>(current_def->def.usr, removed, added)); \
    } \
  }

  // File
  files_def_update.push_back(BuildFileDef(current_file));

  // Types
  CompareGroups<IndexedTypeDef>(previous_file.types, current_file.types,
    /*onRemoved:*/[this](IndexedTypeDef* def) {
    types_removed.push_back(def->def.usr);
  },
    /*onAdded:*/[this, &current_file](IndexedTypeDef* def) {
    QueryableTypeDef query(current_file.id_cache, *def);
    types_def_update.push_back(query.def);
    types_derived.push_back(QueryableTypeDef::DerivedUpdate(query.def.usr, query.derived));
    types_instantiations.push_back(QueryableTypeDef::InstantiationsUpdate(query.def.usr, query.instantiations));
    types_uses.push_back(QueryableTypeDef::UsesUpdate(query.def.usr, query.uses));
  },
    /*onFound:*/[this, &previous_file, &current_file](IndexedTypeDef* previous_def, IndexedTypeDef* current_def) {
    QueryableTypeDef::DefUpdate previous_remapped_def = MapIdToUsr(previous_file.id_cache, previous_def->def);
    QueryableTypeDef::DefUpdate current_remapped_def = MapIdToUsr(current_file.id_cache, current_def->def);
    if (previous_remapped_def != current_remapped_def)
      types_def_update.push_back(current_remapped_def);

    PROCESS_UPDATE_DIFF(types_derived, derived, Usr);
    PROCESS_UPDATE_DIFF(types_instantiations, instantiations, Usr);
    PROCESS_UPDATE_DIFF(types_uses, uses, QueryableLocation);
  });

  // Functions
  CompareGroups<IndexedFuncDef>(previous_file.funcs, current_file.funcs,
    /*onRemoved:*/[this](IndexedFuncDef* def) {
    funcs_removed.push_back(def->def.usr);
  },
    /*onAdded:*/[this, &current_file](IndexedFuncDef* def) {
    QueryableFuncDef query(current_file.id_cache, *def);
    funcs_def_update.push_back(query.def);
    funcs_declarations.push_back(QueryableFuncDef::DeclarationsUpdate(query.def.usr, query.declarations));
    funcs_derived.push_back(QueryableFuncDef::DerivedUpdate(query.def.usr, query.derived));
    funcs_callers.push_back(QueryableFuncDef::CallersUpdate(query.def.usr, query.callers));
    funcs_uses.push_back(QueryableFuncDef::UsesUpdate(query.def.usr, query.uses));
  },
    /*onFound:*/[this, &previous_file, &current_file](IndexedFuncDef* previous_def, IndexedFuncDef* current_def) {
    QueryableFuncDef::DefUpdate previous_remapped_def = MapIdToUsr(previous_file.id_cache, previous_def->def);
    QueryableFuncDef::DefUpdate current_remapped_def = MapIdToUsr(current_file.id_cache, current_def->def);
    if (previous_remapped_def != current_remapped_def)
      funcs_def_update.push_back(current_remapped_def);

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
    QueryableVarDef query(current_file.id_cache, *def);
    vars_def_update.push_back(query.def);
    vars_uses.push_back(QueryableVarDef::UsesUpdate(query.def.usr, query.uses));
  },
    /*onFound:*/[this, &previous_file, &current_file](IndexedVarDef* previous_def, IndexedVarDef* current_def) {
    QueryableVarDef::DefUpdate previous_remapped_def = MapIdToUsr(previous_file.id_cache, previous_def->def);
    QueryableVarDef::DefUpdate current_remapped_def = MapIdToUsr(current_file.id_cache, current_def->def);
    if (previous_remapped_def != current_remapped_def)
      vars_def_update.push_back(current_remapped_def);

    PROCESS_UPDATE_DIFF(vars_uses, uses, QueryableLocation);
  });

#undef PROCESS_UPDATE_DIFF
}

void IndexUpdate::Merge(const IndexUpdate& update) {
#define INDEX_UPDATE_MERGE(name) \
    AddRange(&name, update.name);

  INDEX_UPDATE_MERGE(files_removed);
  INDEX_UPDATE_MERGE(files_def_update);

  INDEX_UPDATE_MERGE(types_removed);
  INDEX_UPDATE_MERGE(types_def_update);
  INDEX_UPDATE_MERGE(types_derived);
  INDEX_UPDATE_MERGE(types_instantiations);
  INDEX_UPDATE_MERGE(types_uses);

  INDEX_UPDATE_MERGE(funcs_removed);
  INDEX_UPDATE_MERGE(funcs_def_update);
  INDEX_UPDATE_MERGE(funcs_declarations);
  INDEX_UPDATE_MERGE(funcs_derived);
  INDEX_UPDATE_MERGE(funcs_callers);
  INDEX_UPDATE_MERGE(funcs_uses);

  INDEX_UPDATE_MERGE(vars_removed);
  INDEX_UPDATE_MERGE(vars_def_update);
  INDEX_UPDATE_MERGE(vars_uses);

#undef INDEX_UPDATE_MERGE
}
























void QueryableDatabase::RemoveUsrs(const std::vector<Usr>& to_remove) {
  // TODO: Removing usrs is tricky because it means we will have to rebuild idx locations. I'm thinking we just nullify
  //       the entry instead of actually removing the data. The index could be massive.
  for (Usr usr : to_remove)
    usr_to_symbol[usr].kind = SymbolKind::Invalid;
  // TODO: also remove from qualified_names?
}

void QueryableDatabase::ImportOrUpdate(const std::vector<QueryableFile::DefUpdate>& updates) {
  for (auto& def : updates) {
    auto it = usr_to_symbol.find(def.usr);
    if (it == usr_to_symbol.end()) {
      qualified_names.push_back(def.usr);
      symbols.push_back(SymbolIdx(SymbolKind::File, files.size()));
      usr_to_symbol[def.usr] = SymbolIdx(SymbolKind::File, files.size());

      QueryableFile query;
      query.def = def;
      files.push_back(query);
    }
    else {
      QueryableFile& existing = files[it->second.idx];
      existing.def = def;
    }
  }
}

void QueryableDatabase::ImportOrUpdate(const std::vector<QueryableTypeDef::DefUpdate>& updates) {
  for (auto& def : updates) {
    auto it = usr_to_symbol.find(def.usr);
    if (it == usr_to_symbol.end()) {
      qualified_names.push_back(def.qualified_name);
      symbols.push_back(SymbolIdx(SymbolKind::Type, types.size()));
      usr_to_symbol[def.usr] = SymbolIdx(SymbolKind::Type, types.size());

      QueryableTypeDef query;
      query.def = def;
      types.push_back(query);
    }
    else {
      QueryableTypeDef& existing = types[it->second.idx];
      if (def.definition_extent)
        existing.def = def;
    }
  }
}

void QueryableDatabase::ImportOrUpdate(const std::vector<QueryableFuncDef::DefUpdate>& updates) {
  for (auto& def : updates) {
    auto it = usr_to_symbol.find(def.usr);
    if (it == usr_to_symbol.end()) {
      qualified_names.push_back(def.qualified_name);
      symbols.push_back(SymbolIdx(SymbolKind::Func, funcs.size()));
      usr_to_symbol[def.usr] = SymbolIdx(SymbolKind::Func, funcs.size());

      QueryableFuncDef query;
      query.def = def;
      funcs.push_back(query);
    }
    else {
      QueryableFuncDef& existing = funcs[it->second.idx];
      if (def.definition_extent)
        existing.def = def;
    }
  }
}

void QueryableDatabase::ImportOrUpdate(const std::vector<QueryableVarDef::DefUpdate>& updates) {
  for (auto& def : updates) {
    auto it = usr_to_symbol.find(def.usr);
    if (it == usr_to_symbol.end()) {
      qualified_names.push_back(def.qualified_name);
      symbols.push_back(SymbolIdx(SymbolKind::Var, vars.size()));
      usr_to_symbol[def.usr] = SymbolIdx(SymbolKind::Var, vars.size());

      QueryableVarDef query;
      query.def = def;
      vars.push_back(query);
    }
    else {
      QueryableVarDef& existing = vars[it->second.idx];
      if (def.definition_extent)
        existing.def = def;
    }
  }
}

void QueryableDatabase::ApplyIndexUpdate(IndexUpdate* update) {
#define HANDLE_REPLACEMENT(update_var_name, def_var_name, storage_name) \
  for (auto replacement_update : update->update_var_name) { \
    SymbolIdx index = usr_to_symbol[replacement_update.usr]; \
    auto* def = &storage_name[index.idx]; \
    def->def_var_name = replacement_update.entries; \
  }

#define HANDLE_MERGEABLE(update_var_name, def_var_name, storage_name) \
  for (auto merge_update : update->update_var_name) { \
    SymbolIdx index = usr_to_symbol[merge_update.usr]; \
    auto* def = &storage_name[index.idx]; \
    AddRange(&def->def_var_name, merge_update.to_add); \
    RemoveRange(&def->def_var_name, merge_update.to_remove); \
  }

  RemoveUsrs(update->files_removed);
  ImportOrUpdate(update->files_def_update);

  RemoveUsrs(update->types_removed);
  ImportOrUpdate(update->types_def_update);
  HANDLE_MERGEABLE(types_derived, derived, types);
  HANDLE_MERGEABLE(types_uses, uses, types);

  RemoveUsrs(update->funcs_removed);
  ImportOrUpdate(update->funcs_def_update);
  HANDLE_MERGEABLE(funcs_declarations, declarations, funcs);
  HANDLE_MERGEABLE(funcs_derived, derived, funcs);
  HANDLE_MERGEABLE(funcs_callers, callers, funcs);
  HANDLE_MERGEABLE(funcs_uses, uses, funcs);

  RemoveUsrs(update->vars_removed);
  ImportOrUpdate(update->vars_def_update);
  HANDLE_MERGEABLE(vars_uses, uses, vars);

#undef HANDLE_MERGEABLE
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
