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


namespace {

QueryableTypeDef::DefUpdate ToQuery(const IdMap& id_map, const IndexedTypeDef::Def& def) {
  QueryableTypeDef::DefUpdate result(def.usr);
  result.short_name = def.short_name;
  result.qualified_name = def.qualified_name;
  result.definition_spelling = id_map.ToQuery(def.definition_spelling);
  result.definition_extent = id_map.ToQuery(def.definition_extent);
  result.alias_of = id_map.ToQuery(def.alias_of);
  result.parents = id_map.ToQuery(def.parents);
  result.types = id_map.ToQuery(def.types);
  result.funcs = id_map.ToQuery(def.funcs);
  result.vars = id_map.ToQuery(def.vars);
  return result;
}

QueryableFuncDef::DefUpdate ToQuery(const IdMap& id_map, const IndexedFuncDef::Def& def) {
  QueryableFuncDef::DefUpdate result(def.usr);
  result.short_name = def.short_name;
  result.qualified_name = def.qualified_name;
  result.definition_spelling = id_map.ToQuery(def.definition_spelling);
  result.definition_extent = id_map.ToQuery(def.definition_extent);
  result.declaring_type = id_map.ToQuery(def.declaring_type);
  result.base = id_map.ToQuery(def.base);
  result.locals = id_map.ToQuery(def.locals);
  result.callees = id_map.ToQuery(def.callees);
  return result;
}

QueryableVarDef::DefUpdate ToQuery(const IdMap& id_map, const IndexedVarDef::Def& def) {
  QueryableVarDef::DefUpdate result(def.usr);
  result.short_name = def.short_name;
  result.qualified_name = def.qualified_name;
  result.declaration = id_map.ToQuery(def.declaration);
  result.definition_spelling = id_map.ToQuery(def.definition_spelling);
  result.definition_extent = id_map.ToQuery(def.definition_extent);
  result.variable_type = id_map.ToQuery(def.variable_type);
  result.declaring_type = id_map.ToQuery(def.declaring_type);
  return result;
}

}  // namespace












QueryableFile::Def BuildFileDef(const IdMap& id_map, const IndexedFile& indexed) {
  QueryableFile::Def def;
  def.usr = indexed.path;

  auto add_outline = [&def, &id_map](SymbolIdx idx, Range range) {
    def.outline.push_back(SymbolRef(idx, id_map.ToQuery(range)));
  };
  auto add_all_symbols = [&def, &id_map](SymbolIdx idx, Range range) {
    def.all_symbols.push_back(SymbolRef(idx, id_map.ToQuery(range)));
  };

  for (const IndexedTypeDef& def : indexed.types) {
    if (def.def.definition_spelling.has_value())
      add_all_symbols(id_map.ToSymbol(def.id), def.def.definition_spelling.value());
    if (def.def.definition_extent.has_value())
      add_outline(id_map.ToSymbol(def.id), def.def.definition_extent.value());
    for (const Range& use : def.uses)
      add_all_symbols(id_map.ToSymbol(def.id), use);
  }
  for (const IndexedFuncDef& def : indexed.funcs) {
    if (def.def.definition_spelling.has_value())
      add_all_symbols(id_map.ToSymbol(def.id), def.def.definition_spelling.value());
    if (def.def.definition_extent.has_value())
      add_outline(id_map.ToSymbol(def.id), def.def.definition_extent.value());
    for (Range decl : def.declarations) {
      add_outline(id_map.ToSymbol(def.id), decl);
      add_all_symbols(id_map.ToSymbol(def.id), decl);
    }
    for (const Range& use : def.uses)
      add_all_symbols(id_map.ToSymbol(def.id), use);
  }
  for (const IndexedVarDef& def : indexed.vars) {
    if (def.def.definition_spelling.has_value())
      add_all_symbols(id_map.ToSymbol(def.id), def.def.definition_spelling.value());
    if (def.def.definition_extent.has_value())
      add_outline(id_map.ToSymbol(def.id), def.def.definition_extent.value());
    for (const Range& use : def.uses)
      add_all_symbols(id_map.ToSymbol(def.id), use);
  }

  std::sort(def.outline.begin(), def.outline.end(), [](const SymbolRef& a, const SymbolRef& b) {
    return a.loc.range.start < b.loc.range.start;
  });
  std::sort(def.all_symbols.begin(), def.all_symbols.end(), [](const SymbolRef& a, const SymbolRef& b) {
    return a.loc.range.start < b.loc.range.start;
  });

  return def;
}

QueryableFile::QueryableFile(const IdMap& id_map, const IndexedFile& indexed)
  : def(BuildFileDef(id_map, indexed)) {}

QueryableTypeDef::QueryableTypeDef(const IdMap& id_map, const IndexedTypeDef& indexed)
  : def(ToQuery(id_map, indexed.def)) {
  derived = id_map.ToQuery(indexed.derived);
  instantiations = id_map.ToQuery(indexed.instantiations);
  uses = id_map.ToQuery(indexed.uses);
}

QueryableFuncDef::QueryableFuncDef(const IdMap& id_map, const IndexedFuncDef& indexed)
  : def(ToQuery(id_map, indexed.def)) {
  declarations = id_map.ToQuery(indexed.declarations);
  derived = id_map.ToQuery(indexed.derived);
  callers = id_map.ToQuery(indexed.callers);
  uses = id_map.ToQuery(indexed.uses);
}

QueryableVarDef::QueryableVarDef(const IdMap& id_map, const IndexedVarDef& indexed)
  : def(ToQuery(id_map, indexed.def)) {
  uses = id_map.ToQuery(indexed.uses);
}
















QueryableFile* SymbolIdx::ResolveFile(QueryableDatabase* db) const {
  assert(kind == SymbolKind::File);
  return &db->files[idx];
}
QueryableTypeDef* SymbolIdx::ResolveType(QueryableDatabase* db) const {
  assert(kind == SymbolKind::Type);
  return &db->types[idx];
}
QueryableFuncDef* SymbolIdx::ResolveFunc(QueryableDatabase* db) const {
  assert(kind == SymbolKind::Func);
  return &db->funcs[idx];
}
QueryableVarDef* SymbolIdx::ResolveVar(QueryableDatabase* db) const {
  assert(kind == SymbolKind::Var);
  return &db->vars[idx];
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
// TODO: Figure out where the invalid SymbolKinds are coming from.
QueryFileId GetQueryFileIdFromUsr(QueryableDatabase* query_db, const Usr& usr) {
  auto it = query_db->usr_to_symbol.find(usr);
  if (it != query_db->usr_to_symbol.end() && it->second.kind != SymbolKind::Invalid) {
    assert(it->second.kind == SymbolKind::File);
    return QueryFileId(it->second.idx);
  }

  size_t idx = query_db->files.size();
  query_db->usr_to_symbol[usr] = SymbolIdx(SymbolKind::File, idx);
  query_db->files.push_back(QueryableFile(usr));
  return QueryFileId(idx);
}

QueryTypeId GetQueryTypeIdFromUsr(QueryableDatabase* query_db, const Usr& usr) {
  auto it = query_db->usr_to_symbol.find(usr);
  if (it != query_db->usr_to_symbol.end() && it->second.kind != SymbolKind::Invalid) {
    assert(it->second.kind == SymbolKind::Type);
    return QueryTypeId(it->second.idx);
  }

  size_t idx = query_db->types.size();
  query_db->usr_to_symbol[usr] = SymbolIdx(SymbolKind::Type, idx);
  query_db->types.push_back(QueryableTypeDef(usr));
  return QueryTypeId(idx);
}

QueryFuncId GetQueryFuncIdFromUsr(QueryableDatabase* query_db, const Usr& usr) {
  auto it = query_db->usr_to_symbol.find(usr);
  if (it != query_db->usr_to_symbol.end() && it->second.kind != SymbolKind::Invalid) {
    assert(it->second.kind == SymbolKind::Func);
    return QueryFuncId(it->second.idx);
  }

  size_t idx = query_db->funcs.size();
  query_db->usr_to_symbol[usr] = SymbolIdx(SymbolKind::Func, idx);
  query_db->funcs.push_back(QueryableFuncDef(usr));
  return QueryFuncId(idx);
}

QueryVarId GetQueryVarIdFromUsr(QueryableDatabase* query_db, const Usr& usr) {
  auto it = query_db->usr_to_symbol.find(usr);
  if (it != query_db->usr_to_symbol.end() && it->second.kind != SymbolKind::Invalid) {
    assert(it->second.kind == SymbolKind::Var);
    return QueryVarId(it->second.idx);
  }

  size_t idx = query_db->vars.size();
  query_db->usr_to_symbol[usr] = SymbolIdx(SymbolKind::Var, idx);
  query_db->vars.push_back(QueryableVarDef(usr));
  return QueryVarId(idx);
}

IdMap::IdMap(QueryableDatabase* query_db, const IdCache& local_ids)
  : local_ids(local_ids) {
  primary_file = GetQueryFileIdFromUsr(query_db, local_ids.primary_file);

  cached_type_ids_.set_empty_key(-1);
  cached_type_ids_.resize(local_ids.type_id_to_usr.size());
  for (const auto& entry : local_ids.type_id_to_usr)
    cached_type_ids_[entry.first.id] = GetQueryTypeIdFromUsr(query_db, entry.second).id;

  cached_func_ids_.set_empty_key(-1);
  cached_func_ids_.resize(local_ids.func_id_to_usr.size());
  for (const auto& entry : local_ids.func_id_to_usr)
    cached_func_ids_[entry.first.id] = GetQueryFuncIdFromUsr(query_db, entry.second).id;

  cached_var_ids_.set_empty_key(-1);
  cached_var_ids_.resize(local_ids.var_id_to_usr.size());
  for (const auto& entry : local_ids.var_id_to_usr)
    cached_var_ids_[entry.first.id] = GetQueryVarIdFromUsr(query_db, entry.second).id;
}

QueryableLocation IdMap::ToQuery(Range range) const {
  return QueryableLocation(primary_file, range);
}

QueryTypeId IdMap::ToQuery(IndexTypeId id) const {
  assert(cached_type_ids_.find(id.id) != cached_type_ids_.end());
  return QueryTypeId(cached_type_ids_.find(id.id)->second);
}
QueryFuncId IdMap::ToQuery(IndexFuncId id) const {
  assert(cached_func_ids_.find(id.id) != cached_func_ids_.end());
  return QueryFuncId(cached_func_ids_.find(id.id)->second);
}
QueryVarId IdMap::ToQuery(IndexVarId id) const {
  assert(cached_var_ids_.find(id.id) != cached_var_ids_.end());
  return QueryVarId(cached_var_ids_.find(id.id)->second);
}
QueryFuncRef IdMap::ToQuery(IndexFuncRef ref) const {
  return QueryFuncRef(ToQuery(ref.id), ToQuery(ref.loc));
}

optional<QueryableLocation> IdMap::ToQuery(optional<Range> range) const {
  if (!range)
    return nullopt;
  return ToQuery(range.value());
}
optional<QueryTypeId> IdMap::ToQuery(optional<IndexTypeId> id) const {
  if (!id)
    return nullopt;
  return ToQuery(id.value());
}
optional<QueryFuncId> IdMap::ToQuery(optional<IndexFuncId> id) const {
  if (!id)
    return nullopt;
  return ToQuery(id.value());
}
optional<QueryVarId> IdMap::ToQuery(optional<IndexVarId> id) const {
  if (!id)
    return nullopt;
  return ToQuery(id.value());
}
optional<QueryFuncRef> IdMap::ToQuery(optional<IndexFuncRef> ref) const {
  if (!ref)
    return nullopt;
  return ToQuery(ref.value());
}

template<typename In, typename Out>
std::vector<Out> ToQueryTransform(const IdMap& id_map, const std::vector<In>& input) {
  std::vector<Out> result;
  result.reserve(input.size());
  for (const In& in : input)
    result.push_back(id_map.ToQuery(in));
  return result;
}
std::vector<QueryableLocation> IdMap::ToQuery(std::vector<Range> ranges) const {
  return ToQueryTransform<Range, QueryableLocation>(*this, ranges);
}
std::vector<QueryTypeId> IdMap::ToQuery(std::vector<IndexTypeId> ids) const {
  return ToQueryTransform<IndexTypeId, QueryTypeId>(*this, ids);
}
std::vector<QueryFuncId> IdMap::ToQuery(std::vector<IndexFuncId> ids) const {
  return ToQueryTransform<IndexFuncId, QueryFuncId>(*this, ids);
}
std::vector<QueryVarId> IdMap::ToQuery(std::vector<IndexVarId> ids) const {
  return ToQueryTransform<IndexVarId, QueryVarId>(*this, ids);
}
std::vector<QueryFuncRef> IdMap::ToQuery(std::vector<IndexFuncRef> refs) const {
  return ToQueryTransform<IndexFuncRef, QueryFuncRef>(*this, refs);
}

SymbolIdx IdMap::ToSymbol(IndexTypeId id) const {
  return SymbolIdx(SymbolKind::Type, ToQuery(id).id);
}
SymbolIdx IdMap::ToSymbol(IndexFuncId id) const {
  return SymbolIdx(SymbolKind::Func, ToQuery(id).id);
}
SymbolIdx IdMap::ToSymbol(IndexVarId id) const {
  return SymbolIdx(SymbolKind::Var, ToQuery(id).id);
}

















// static
IndexUpdate IndexUpdate::CreateDelta(const IdMap* previous_id_map, const IdMap* current_id_map, IndexedFile* previous, IndexedFile* current) {
  if (!previous_id_map) {
    assert(!previous);
    IndexedFile previous(current->path);
    return IndexUpdate(*current_id_map, *current_id_map, previous, *current);
  }
  return IndexUpdate(*previous_id_map, *current_id_map, *previous, *current);
}

IndexUpdate::IndexUpdate(const IdMap& previous_id_map, const IdMap& current_id_map, IndexedFile& previous_file, IndexedFile& current_file) {
  // |query_name| is the name of the variable on the query type.
  // |index_name| is the name of the variable on the index type.
  // |type| is the type of the variable.
#define PROCESS_UPDATE_DIFF(query_name, index_name, type) \
  { \
    /* Check for changes. */ \
    std::vector<type> removed, added; \
    auto previous = previous_id_map.ToQuery(previous_def->index_name); \
    auto current = current_id_map.ToQuery(current_def->index_name); \
    bool did_add = ComputeDifferenceForUpdate( \
                      previous, current, \
                      &removed, &added); \
    if (did_add) {\
      /*std::cerr << "Adding mergeable update on " << current_def->def.short_name << " (" << current_def->def.usr << ") for field " << #index_name << std::endl;*/ \
      query_name.push_back(MergeableUpdate<type>(current_def->def.usr, added, removed)); \
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
    QueryableTypeDef::DefUpdate previous_remapped_def = ToQuery(previous_id_map, previous_def->def);
    QueryableTypeDef::DefUpdate current_remapped_def = ToQuery(current_id_map, current_def->def);
    if (previous_remapped_def != current_remapped_def)
      types_def_update.push_back(current_remapped_def);

    PROCESS_UPDATE_DIFF(types_derived, derived, QueryTypeId);
    PROCESS_UPDATE_DIFF(types_instantiations, instantiations, QueryVarId);
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
    QueryableFuncDef::DefUpdate previous_remapped_def = ToQuery(previous_id_map, previous_def->def);
    QueryableFuncDef::DefUpdate current_remapped_def = ToQuery(current_id_map, current_def->def);
    if (previous_remapped_def != current_remapped_def)
      funcs_def_update.push_back(current_remapped_def);

    PROCESS_UPDATE_DIFF(funcs_declarations, declarations, QueryableLocation);
    PROCESS_UPDATE_DIFF(funcs_derived, derived, QueryFuncId);
    PROCESS_UPDATE_DIFF(funcs_callers, callers, QueryFuncRef);
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
    QueryableVarDef::DefUpdate previous_remapped_def = ToQuery(previous_id_map, previous_def->def);
    QueryableVarDef::DefUpdate current_remapped_def = ToQuery(current_id_map, current_def->def);
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



















void UpdateQualifiedName(QueryableDatabase* db, size_t* qualified_name_index, SymbolKind kind, size_t symbol_index, const std::string& name) {
  if (*qualified_name_index == -1) {
    db->qualified_names.push_back(name);
    db->symbols.push_back(SymbolIdx(kind, symbol_index));
    *qualified_name_index = db->qualified_names.size() - 1;
  }
  else {
    db->qualified_names[*qualified_name_index] = name;
  }
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
    assert(it != usr_to_symbol.end());

    QueryableFile& existing = files[it->second.idx];
    existing.def = def;
    //UpdateQualifiedName(this, &existing.qualified_name_idx, SymbolKind::File, it->second.idx, def.usr);
  }
}

void QueryableDatabase::ImportOrUpdate(const std::vector<QueryableTypeDef::DefUpdate>& updates) {
  for (auto& def : updates) {
    if (!def.definition_extent)
      continue;

    auto it = usr_to_symbol.find(def.usr);
    assert(it != usr_to_symbol.end());

    QueryableTypeDef& existing = types[it->second.idx];
    existing.def = def;
    UpdateQualifiedName(this, &existing.qualified_name_idx, SymbolKind::Type, it->second.idx, def.usr);
  }
}

void QueryableDatabase::ImportOrUpdate(const std::vector<QueryableFuncDef::DefUpdate>& updates) {
  for (auto& def : updates) {
    if (!def.definition_extent)
      continue;

    auto it = usr_to_symbol.find(def.usr);
    assert(it != usr_to_symbol.end());

    QueryableFuncDef& existing = funcs[it->second.idx];
    existing.def = def;
    UpdateQualifiedName(this, &existing.qualified_name_idx, SymbolKind::Func, it->second.idx, def.usr);
  }
}

void QueryableDatabase::ImportOrUpdate(const std::vector<QueryableVarDef::DefUpdate>& updates) {
  for (auto& def : updates) {
    if (!def.definition_extent)
      continue;

    auto it = usr_to_symbol.find(def.usr);
    assert(it != usr_to_symbol.end());

    QueryableVarDef& existing = vars[it->second.idx];
    existing.def = def;
    if (def.declaring_type)
      UpdateQualifiedName(this, &existing.qualified_name_idx, SymbolKind::Var, it->second.idx, def.usr);
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
