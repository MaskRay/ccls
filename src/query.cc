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







Usr MapIdToUsr(const IdMap& id_map, const IndexTypeId& id) {
  assert(id_map.local_ids.type_id_to_usr.find(id) != id_map.local_ids.type_id_to_usr.end());
  return id_map.local_ids.type_id_to_usr.find(id)->second;
}
Usr MapIdToUsr(const IdMap& id_map, const IndexFuncId& id) {
  assert(id_map.local_ids.func_id_to_usr.find(id) != id_map.local_ids.func_id_to_usr.end());
  return id_map.local_ids.func_id_to_usr.find(id)->second;
}
Usr MapIdToUsr(const IdMap& id_map, const IndexVarId& id) {
  assert(id_map.local_ids.var_id_to_usr.find(id) != id_map.local_ids.var_id_to_usr.end());
  return id_map.local_ids.var_id_to_usr.find(id)->second;
}
QueryableLocation MapIdToUsr(const IdMap& id_map, const Range& range) {
  return QueryableLocation(id_map.local_ids.primary_file, range);
}
UsrRef MapIdToUsr(const IdMap& id_map, const FuncRef& id) {
  assert(id_map.local_ids.func_id_to_usr.find(id.id) != id_map.local_ids.func_id_to_usr.end());
  return UsrRef(
    id_map.local_ids.func_id_to_usr.find(id.id)->second /*usr*/,
    MapIdToUsr(id_map, id.loc) /*loc*/);
}

// Mapps for vectors of elements. We have to explicitly instantiate each
// template instance because C++ cannot deduce the return type template
// parameter.
template<typename In, typename Out>
std::vector<Out> Transform(const IdMap& id_map, const std::vector<In>& input) {
  std::vector<Out> result;
  result.reserve(input.size());
  for (const In& in : input)
    result.push_back(MapIdToUsr(id_map, in));
  return result;
}
std::vector<Usr> MapIdToUsr(const IdMap& id_map, const std::vector<IndexTypeId>& ids) {
  return Transform<IndexTypeId, Usr>(id_map, ids);
}
std::vector<Usr> MapIdToUsr(const IdMap& id_map, const std::vector<IndexFuncId>& ids) {
  return Transform<IndexFuncId, Usr>(id_map, ids);
}
std::vector<Usr> MapIdToUsr(const IdMap& id_map, const std::vector<IndexVarId>& ids) {
  return Transform<IndexVarId, Usr>(id_map, ids);
}
std::vector<UsrRef> MapIdToUsr(const IdMap& id_map, const std::vector<FuncRef>& ids) {
  return Transform<FuncRef, UsrRef>(id_map, ids);
}
std::vector<QueryableLocation> MapIdToUsr(const IdMap& id_map, const std::vector<Range>& ids) {
  return Transform<Range, QueryableLocation>(id_map, ids);
}



QueryableTypeDef::DefUpdate MapIdToUsr(const IdMap& id_map, const IndexedTypeDef::Def& def) {
  QueryableTypeDef::DefUpdate result(def.usr);
  result.short_name = def.short_name;
  result.qualified_name = def.qualified_name;
  if (def.definition_spelling)
    result.definition_spelling = MapIdToUsr(id_map, def.definition_spelling.value());
  if (def.definition_extent)
    result.definition_extent = MapIdToUsr(id_map, def.definition_extent.value());
  if (def.alias_of)
    result.alias_of = MapIdToUsr(id_map, def.alias_of.value());
  result.parents = MapIdToUsr(id_map, def.parents);
  result.types = MapIdToUsr(id_map, def.types);
  result.funcs = MapIdToUsr(id_map, def.funcs);
  result.vars = MapIdToUsr(id_map, def.vars);
  return result;
}
QueryableFuncDef::DefUpdate MapIdToUsr(const IdMap& id_map, const IndexedFuncDef::Def& def) {
  QueryableFuncDef::DefUpdate result(def.usr);
  result.short_name = def.short_name;
  result.qualified_name = def.qualified_name;
  if (def.definition_spelling)
    result.definition_spelling = MapIdToUsr(id_map, def.definition_spelling.value());
  if (def.definition_extent)
    result.definition_extent = MapIdToUsr(id_map, def.definition_extent.value());
  if (def.declaring_type)
    result.declaring_type = MapIdToUsr(id_map, def.declaring_type.value());
  if (def.base)
    result.base = MapIdToUsr(id_map, def.base.value());
  result.locals = MapIdToUsr(id_map, def.locals);
  result.callees = MapIdToUsr(id_map, def.callees);
  return result;
}
QueryableVarDef::DefUpdate MapIdToUsr(const IdMap& id_map, const IndexedVarDef::Def& def) {
  QueryableVarDef::DefUpdate result(def.usr);
  result.short_name = def.short_name;
  result.qualified_name = def.qualified_name;
  if (def.declaration)
    result.declaration = MapIdToUsr(id_map, def.declaration.value());
  if (def.definition_spelling)
    result.definition_spelling = MapIdToUsr(id_map, def.definition_spelling.value());
  if (def.definition_extent)
    result.definition_extent = MapIdToUsr(id_map, def.definition_extent.value());
  if (def.variable_type)
    result.variable_type = MapIdToUsr(id_map, def.variable_type.value());
  if (def.declaring_type)
    result.declaring_type = MapIdToUsr(id_map, def.declaring_type.value());
  return result;
}













QueryableFile::Def BuildFileDef(const IdMap& id_map, const IndexedFile& indexed) {
  QueryableFile::Def def;
  def.usr = indexed.path;

  auto add_outline = [&def, &id_map](Usr usr, Range range) {
    def.outline.push_back(UsrRef(usr, MapIdToUsr(id_map, range)));
  };
  auto add_all_symbols = [&def, &id_map](Usr usr, Range range) {
    def.all_symbols.push_back(UsrRef(usr, MapIdToUsr(id_map, range)));
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

QueryableFile::QueryableFile(const IdMap& id_map, const IndexedFile& indexed)
  : def(BuildFileDef(id_map, indexed)) {}

QueryableTypeDef::QueryableTypeDef(const IdMap& id_map, const IndexedTypeDef& indexed)
  : def(MapIdToUsr(id_map, indexed.def)) {
  derived = MapIdToUsr(id_map, indexed.derived);
  instantiations = MapIdToUsr(id_map, indexed.instantiations);
  uses = MapIdToUsr(id_map, indexed.uses);
}

QueryableFuncDef::QueryableFuncDef(const IdMap& id_map, const IndexedFuncDef& indexed)
  : def(MapIdToUsr(id_map, indexed.def)) {
  declarations = MapIdToUsr(id_map, indexed.declarations);
  derived = MapIdToUsr(id_map, indexed.derived);
  callers = MapIdToUsr(id_map, indexed.callers);
  uses = MapIdToUsr(id_map, indexed.uses);
}

QueryableVarDef::QueryableVarDef(const IdMap& id_map, const IndexedVarDef& indexed)
  : def(MapIdToUsr(id_map, indexed.def)) {
  uses = MapIdToUsr(id_map, indexed.uses);
}











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
  // Returns the elements in |current| that are not in |previous|.
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
      on_found(&*prev_it, &*curr_it);
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






// TODO: consider having separate lookup maps so they are smaller (maybe
// lookups will go faster).

QueryFileId GetQueryFileIdFromUsr(QueryableDatabase* query_db, const Usr& usr) {
  auto it = query_db->usr_to_symbol.find(usr);
  if (it != query_db->usr_to_symbol.end()) {
    assert(it->second.kind == SymbolKind::File);
    return it->second.idx;
  }

  int idx = query_db->files.size();
  query_db->usr_to_symbol[usr] = SymbolIdx(SymbolKind::File, idx);
  query_db->files.push_back(QueryableFile(usr));
  return idx;
}

QueryTypeId GetQueryTypeIdFromUsr(QueryableDatabase* query_db, const Usr& usr) {
  auto it = query_db->usr_to_symbol.find(usr);
  if (it != query_db->usr_to_symbol.end()) {
    assert(it->second.kind == SymbolKind::Type);
    return it->second.idx;
  }

  int idx = query_db->types.size();
  query_db->usr_to_symbol[usr] = SymbolIdx(SymbolKind::Type, idx);
  query_db->types.push_back(QueryableTypeDef(usr));
  return idx;
}

QueryFuncId GetQueryFuncIdFromUsr(QueryableDatabase* query_db, const Usr& usr) {
  auto it = query_db->usr_to_symbol.find(usr);
  if (it != query_db->usr_to_symbol.end()) {
    assert(it->second.kind == SymbolKind::Func);
    return it->second.idx;
  }

  int idx = query_db->funcs.size();
  query_db->usr_to_symbol[usr] = SymbolIdx(SymbolKind::Func, idx);
  query_db->funcs.push_back(QueryableFuncDef(usr));
  return idx;
}

QueryVarId GetQueryVarIdFromUsr(QueryableDatabase* query_db, const Usr& usr) {
  auto it = query_db->usr_to_symbol.find(usr);
  if (it != query_db->usr_to_symbol.end()) {
    assert(it->second.kind == SymbolKind::Var);
    return it->second.idx;
  }

  int idx = query_db->vars.size();
  query_db->usr_to_symbol[usr] = SymbolIdx(SymbolKind::Var, idx);
  query_db->vars.push_back(QueryableVarDef(usr));
  return idx;
}


#if false
int GetOrAddSymbol(QueryableDatabase* query_db, SymbolKind kind, const Usr& usr) {
  // TODO: consider having separate lookup maps so they are smaller (maybe
  // lookups will go faster).
  auto it = query_db->usr_to_symbol.find(usr);

  // Found; return existing symbol.
  if (it != query_db->usr_to_symbol.end()) {
    assert(it->second.kind == kind);
    return it->second.idx;
  }

  // Not found; add a new symbol.
  switch (kind) {
  case SymbolKind::File: {
    int idx = query_db->files.size();
    query_db->usr_to_symbol[usr] = SymbolIdx(kind, idx);
    query_db->files.push_back(QueryableFile(usr));
    return idx;
  }
  case SymbolKind::Type: {
    int idx = query_db->types.size();
    query_db->usr_to_symbol[usr] = SymbolIdx(kind, idx);
    query_db->types.push_back(QueryableTypeDef(usr));
    return idx;
  }
  case SymbolKind::Func: {
    int idx = query_db->funcs.size();
    query_db->usr_to_symbol[usr] = SymbolIdx(kind, idx);
    query_db->funcs.push_back(QueryableFuncDef(usr));
    return idx;
  }
  case SymbolKind::Var: {
    int idx = query_db->vars.size();
    query_db->usr_to_symbol[usr] = SymbolIdx(kind, idx);
    query_db->vars.push_back(QueryableVarDef(usr));
    return idx;
  }
  case SymbolKind::Invalid: {
    assert(false);
    return -1;
  }
  }

  assert(false);
  return -1;
}
#endif

IdMap::IdMap(QueryableDatabase* query_db, const IdCache& local_ids)
  : local_ids(local_ids) {
  assert(query_db); // TODO: remove after testing.

  index_file_id = GetQueryFileIdFromUsr(query_db, local_ids.primary_file);
  
  cached_type_ids_.reserve(local_ids.type_id_to_usr.size());
  for (const auto& entry : local_ids.type_id_to_usr)
    cached_type_ids_[entry.first] = GetQueryTypeIdFromUsr(query_db, entry.second);

  cached_func_ids_.reserve(local_ids.func_id_to_usr.size());
  for (const auto& entry : local_ids.func_id_to_usr)
    cached_func_ids_[entry.first] = GetQueryFuncIdFromUsr(query_db, entry.second);

  cached_var_ids_.reserve(local_ids.var_id_to_usr.size());
  for (const auto& entry : local_ids.var_id_to_usr)
    cached_var_ids_[entry.first] = GetQueryVarIdFromUsr(query_db, entry.second);
}




















// static
IndexUpdate IndexUpdate::CreateImport(const IdMap& id_map, IndexedFile& file) {
  // Return standard diff constructor but with an empty file so everything is
  // added.
  IndexedFile previous(file.path);
  return IndexUpdate(id_map, id_map, previous, file);
}

// static
IndexUpdate IndexUpdate::CreateDelta(const IdMap& current_id_map, const IdMap& previous_id_map, IndexedFile& current, IndexedFile& updated) {
  return IndexUpdate(current_id_map, previous_id_map, current, updated);
}

IndexUpdate::IndexUpdate(const IdMap& current_id_map, const IdMap& previous_id_map, IndexedFile& previous_file, IndexedFile& current_file) {
  // |query_name| is the name of the variable on the query type.
  // |index_name| is the name of the variable on the index type.
  // |type| is the type of the variable.
#define PROCESS_UPDATE_DIFF(query_name, index_name, type) \
  { \
    /* Check for changes. */ \
    std::vector<type> removed, added; \
    auto previous = MapIdToUsr(previous_id_map, previous_def->index_name); \
    auto current = MapIdToUsr(current_id_map, current_def->index_name); \
    bool did_add = ComputeDifferenceForUpdate( \
                      previous, current, \
                      &removed, &added); \
    if (did_add) {\
      std::cerr << "Adding mergeable update on " << current_def->def.short_name << " (" << current_def->def.usr << ") for field " << #index_name << std::endl; \
      query_name.push_back(MergeableUpdate<type>(current_def->def.usr, removed, added)); \
    } \
  }

  // File
  files_def_update.push_back(BuildFileDef(current_id_map, current_file));

  // Types
  CompareGroups<IndexedTypeDef>(previous_file.types, current_file.types,
    /*onRemoved:*/[this](IndexedTypeDef* def) {
    types_removed.push_back(def->def.usr);
  },
    /*onAdded:*/[this, &current_id_map](IndexedTypeDef* def) {
    QueryableTypeDef query(current_id_map, *def);
    types_def_update.push_back(query.def);
    types_derived.push_back(QueryableTypeDef::DerivedUpdate(query.def.usr, query.derived));
    types_instantiations.push_back(QueryableTypeDef::InstantiationsUpdate(query.def.usr, query.instantiations));
    types_uses.push_back(QueryableTypeDef::UsesUpdate(query.def.usr, query.uses));
  },
    /*onFound:*/[this, &previous_id_map, &current_id_map](IndexedTypeDef* previous_def, IndexedTypeDef* current_def) {
    QueryableTypeDef::DefUpdate previous_remapped_def = MapIdToUsr(previous_id_map, previous_def->def);
    QueryableTypeDef::DefUpdate current_remapped_def = MapIdToUsr(current_id_map, current_def->def);
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
    /*onAdded:*/[this, &current_id_map](IndexedFuncDef* def) {
    QueryableFuncDef query(current_id_map, *def);
    funcs_def_update.push_back(query.def);
    funcs_declarations.push_back(QueryableFuncDef::DeclarationsUpdate(query.def.usr, query.declarations));
    funcs_derived.push_back(QueryableFuncDef::DerivedUpdate(query.def.usr, query.derived));
    funcs_callers.push_back(QueryableFuncDef::CallersUpdate(query.def.usr, query.callers));
    funcs_uses.push_back(QueryableFuncDef::UsesUpdate(query.def.usr, query.uses));
  },
    /*onFound:*/[this, &previous_id_map, &current_id_map](IndexedFuncDef* previous_def, IndexedFuncDef* current_def) {
    QueryableFuncDef::DefUpdate previous_remapped_def = MapIdToUsr(previous_id_map, previous_def->def);
    QueryableFuncDef::DefUpdate current_remapped_def = MapIdToUsr(current_id_map, current_def->def);
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
    /*onAdded:*/[this, &current_id_map](IndexedVarDef* def) {
    QueryableVarDef query(current_id_map, *def);
    vars_def_update.push_back(query.def);
    vars_uses.push_back(QueryableVarDef::UsesUpdate(query.def.usr, query.uses));
  },
    /*onFound:*/[this, &previous_id_map, &current_id_map](IndexedVarDef* previous_def, IndexedVarDef* current_def) {
    QueryableVarDef::DefUpdate previous_remapped_def = MapIdToUsr(previous_id_map, previous_def->def);
    QueryableVarDef::DefUpdate current_remapped_def = MapIdToUsr(current_id_map, current_def->def);
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
      files.push_back(QueryableFile(def));
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
      types.push_back(QueryableTypeDef(def));
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
      funcs.push_back(QueryableFuncDef(def));
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
      vars.push_back(QueryableVarDef(def));
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
// TODO: allow user to store configuration as json? file in home dir; also
//       allow local overrides (scan up dirs)
// TODO: add opt to dump config when starting (--dump-config)
// TODO: allow user to decide some indexer choices, ie, do we mark prototype parameters as usages?
