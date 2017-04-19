#include "query.h"

#include "indexer.h"

#include <optional.h>

#include <cassert>
#include <cstdint>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <iostream>

// TODO: Make all copy constructors explicit.


namespace {

QueryType::DefUpdate ToQuery(const IdMap& id_map, const IndexedTypeDef::Def& type) {
  QueryType::DefUpdate result(type.usr);
  result.short_name = type.short_name;
  result.detailed_name = type.detailed_name;
  result.definition_spelling = id_map.ToQuery(type.definition_spelling);
  result.definition_extent = id_map.ToQuery(type.definition_extent);
  result.alias_of = id_map.ToQuery(type.alias_of);
  result.parents = id_map.ToQuery(type.parents);
  result.types = id_map.ToQuery(type.types);
  result.funcs = id_map.ToQuery(type.funcs);
  result.vars = id_map.ToQuery(type.vars);
  return result;
}

QueryFunc::DefUpdate ToQuery(const IdMap& id_map, const IndexedFuncDef::Def& func) {
  QueryFunc::DefUpdate result(func.usr);
  result.short_name = func.short_name;
  result.detailed_name = func.detailed_name;
  result.definition_spelling = id_map.ToQuery(func.definition_spelling);
  result.definition_extent = id_map.ToQuery(func.definition_extent);
  result.declaring_type = id_map.ToQuery(func.declaring_type);
  result.base = id_map.ToQuery(func.base);
  result.locals = id_map.ToQuery(func.locals);
  result.callees = id_map.ToQuery(func.callees);
  return result;
}

QueryVar::DefUpdate ToQuery(const IdMap& id_map, const IndexedVarDef::Def& var) {
  QueryVar::DefUpdate result(var.usr);
  result.short_name = var.short_name;
  result.detailed_name = var.detailed_name;
  result.declaration = id_map.ToQuery(var.declaration);
  result.definition_spelling = id_map.ToQuery(var.definition_spelling);
  result.definition_extent = id_map.ToQuery(var.definition_extent);
  result.variable_type = id_map.ToQuery(var.variable_type);
  result.declaring_type = id_map.ToQuery(var.declaring_type);
  return result;
}


// Adds the mergeable updates in |source| to |dest|. If a mergeable update for
// the destination type already exists, it will be combined. This makes merging
// updates take longer but reduces import time on the querydb thread.
template <typename TId, typename TValue>
void AddMergeableRange(
  std::vector<MergeableUpdate<TId, TValue>>* dest,
  const std::vector<MergeableUpdate<TId, TValue>>& source) {

  // TODO: Consider caching the lookup table. It can probably save even more
  // time at the cost of some additional memory.

  // Build lookup table.
  //google::dense_hash_map<TId, size_t, std::hash<TId>> id_to_index;
  //id_to_index.set_empty_key(TId(-1));
  spp::sparse_hash_map<TId, size_t> id_to_index;
  id_to_index.resize(dest->size());
  for (size_t i = 0; i < dest->size(); ++i)
    id_to_index[(*dest)[i].id] = i;

  // Add entries. Try to add them to an existing entry.
  for (const auto& entry : source) {
    auto it = id_to_index.find(entry.id);
    if (it != id_to_index.end()) {
      AddRange(&(*dest)[it->second].to_add, entry.to_add);
      AddRange(&(*dest)[it->second].to_remove, entry.to_remove);
    }
    else {
      dest->push_back(entry);
    }
  }
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

QueryFile::Def BuildFileDef(const IdMap& id_map, const IndexedFile& indexed) {
  QueryFile::Def def;
  def.path = indexed.path;

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
      add_all_symbols(id_map.ToSymbol(def.id), decl);
      add_outline(id_map.ToSymbol(def.id), decl);
    }
    for (const IndexFuncRef& caller : def.callers)
      add_all_symbols(id_map.ToSymbol(def.id), caller.loc);
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

}  // namespace



























QueryFile* SymbolIdx::ResolveFile(QueryDatabase* db) const {
  assert(kind == SymbolKind::File);
  return &db->files[idx];
}
QueryType* SymbolIdx::ResolveType(QueryDatabase* db) const {
  assert(kind == SymbolKind::Type);
  return &db->types[idx];
}
QueryFunc* SymbolIdx::ResolveFunc(QueryDatabase* db) const {
  assert(kind == SymbolKind::Func);
  return &db->funcs[idx];
}
QueryVar* SymbolIdx::ResolveVar(QueryDatabase* db) const {
  assert(kind == SymbolKind::Var);
  return &db->vars[idx];
}
































// TODO: consider having separate lookup maps so they are smaller (maybe
// lookups will go faster).
QueryFileId GetQueryFileIdFromPath(QueryDatabase* query_db, const std::string& path) {
  auto it = query_db->usr_to_symbol.find(path);
  if (it != query_db->usr_to_symbol.end() && it->second.kind != SymbolKind::Invalid) {
    // TODO: should this be an assert?
    if (it->second.kind == SymbolKind::File)
      return QueryFileId(it->second.idx);
  }

  size_t idx = query_db->files.size();
  query_db->usr_to_symbol[path] = SymbolIdx(SymbolKind::File, idx);
  query_db->files.push_back(QueryFile(path));
  return QueryFileId(idx);
}

QueryTypeId GetQueryTypeIdFromUsr(QueryDatabase* query_db, const Usr& usr) {
  auto it = query_db->usr_to_symbol.find(usr);
  if (it != query_db->usr_to_symbol.end() && it->second.kind != SymbolKind::Invalid) {
    // TODO: should this be an assert?
    if (it->second.kind == SymbolKind::Type)
      return QueryTypeId(it->second.idx);
  }

  size_t idx = query_db->types.size();
  query_db->usr_to_symbol[usr] = SymbolIdx(SymbolKind::Type, idx);
  query_db->types.push_back(QueryType(usr));
  return QueryTypeId(idx);
}

QueryFuncId GetQueryFuncIdFromUsr(QueryDatabase* query_db, const Usr& usr) {
  auto it = query_db->usr_to_symbol.find(usr);
  if (it != query_db->usr_to_symbol.end() && it->second.kind != SymbolKind::Invalid) {
    // TODO: should this be an assert?
    if (it->second.kind == SymbolKind::Func)
      return QueryFuncId(it->second.idx);
  }

  size_t idx = query_db->funcs.size();
  query_db->usr_to_symbol[usr] = SymbolIdx(SymbolKind::Func, idx);
  query_db->funcs.push_back(QueryFunc(usr));
  return QueryFuncId(idx);
}

QueryVarId GetQueryVarIdFromUsr(QueryDatabase* query_db, const Usr& usr) {
  auto it = query_db->usr_to_symbol.find(usr);
  if (it != query_db->usr_to_symbol.end() && it->second.kind != SymbolKind::Invalid) {
    // TODO: should this be an assert?
    if (it->second.kind == SymbolKind::Var)
      return QueryVarId(it->second.idx);
  }

  size_t idx = query_db->vars.size();
  query_db->usr_to_symbol[usr] = SymbolIdx(SymbolKind::Var, idx);
  query_db->vars.push_back(QueryVar(usr));
  return QueryVarId(idx);
}

IdMap::IdMap(QueryDatabase* query_db, const IdCache& local_ids)
  : local_ids(local_ids) {
  primary_file = GetQueryFileIdFromPath(query_db, local_ids.primary_file);

  //cached_type_ids_.set_empty_key(IndexTypeId(-1));
  cached_type_ids_.resize(local_ids.type_id_to_usr.size());
  for (const auto& entry : local_ids.type_id_to_usr)
    cached_type_ids_[entry.first] = GetQueryTypeIdFromUsr(query_db, entry.second);

  //cached_func_ids_.set_empty_key(IndexFuncId(-1));
  cached_func_ids_.resize(local_ids.func_id_to_usr.size());
  for (const auto& entry : local_ids.func_id_to_usr)
    cached_func_ids_[entry.first] = GetQueryFuncIdFromUsr(query_db, entry.second);

  //cached_var_ids_.set_empty_key(IndexVarId(-1));
  cached_var_ids_.resize(local_ids.var_id_to_usr.size());
  for (const auto& entry : local_ids.var_id_to_usr)
    cached_var_ids_[entry.first] = GetQueryVarIdFromUsr(query_db, entry.second);
}

QueryLocation IdMap::ToQuery(Range range) const {
  return QueryLocation(primary_file, range);
}

QueryTypeId IdMap::ToQuery(IndexTypeId id) const {
  assert(cached_type_ids_.find(id) != cached_type_ids_.end());
  return QueryTypeId(cached_type_ids_.find(id)->second);
}
QueryFuncId IdMap::ToQuery(IndexFuncId id) const {
  if (id.id == -1) return QueryFuncId(-1);
  assert(cached_func_ids_.find(id) != cached_func_ids_.end());
  return QueryFuncId(cached_func_ids_.find(id)->second);
}
QueryVarId IdMap::ToQuery(IndexVarId id) const {
  assert(cached_var_ids_.find(id) != cached_var_ids_.end());
  return QueryVarId(cached_var_ids_.find(id)->second);
}
QueryFuncRef IdMap::ToQuery(IndexFuncRef ref) const {
  return QueryFuncRef(ToQuery(ref.id_), ToQuery(ref.loc));
}

optional<QueryLocation> IdMap::ToQuery(optional<Range> range) const {
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
std::vector<QueryLocation> IdMap::ToQuery(std::vector<Range> ranges) const {
  return ToQueryTransform<Range, QueryLocation>(*this, ranges);
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





















// ----------------------
// INDEX THREAD FUNCTIONS
// ----------------------

// static
IndexUpdate IndexUpdate::CreateDelta(const IdMap* previous_id_map, const IdMap* current_id_map, IndexedFile* previous, IndexedFile* current) {
  // This function runs on an indexer thread.

  if (!previous_id_map) {
    assert(!previous);
    IndexedFile previous(current->path);
    return IndexUpdate(*current_id_map, *current_id_map, previous, *current);
  }
  return IndexUpdate(*previous_id_map, *current_id_map, *previous, *current);
}

IndexUpdate::IndexUpdate(const IdMap& previous_id_map, const IdMap& current_id_map, IndexedFile& previous_file, IndexedFile& current_file) {
  // This function runs on an indexer thread.

  // |query_name| is the name of the variable on the query type.
  // |index_name| is the name of the variable on the index type.
  // |type| is the type of the variable.
#define PROCESS_UPDATE_DIFF(type_id, query_name, index_name, type) \
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
      query_name.push_back(MergeableUpdate<type_id, type>(current_id_map.ToQuery(current_def->id), added, removed)); \
    } \
  }
  // File
  files_def_update.push_back(BuildFileDef(current_id_map, current_file));

  // Types
  CompareGroups<IndexedTypeDef>(previous_file.types, current_file.types,
    /*onRemoved:*/[this](IndexedTypeDef* def) {
    types_removed.push_back(def->def.usr);
  },
    /*onAdded:*/[this, &current_id_map](IndexedTypeDef* type) {
    if (!type->def.detailed_name.empty())
      types_def_update.push_back(ToQuery(current_id_map, type->def));
    if (!type->derived.empty())
      types_derived.push_back(QueryType::DerivedUpdate(current_id_map.ToQuery(type->id), current_id_map.ToQuery(type->derived)));
    if (!type->instantiations.empty())
      types_instantiations.push_back(QueryType::InstantiationsUpdate(current_id_map.ToQuery(type->id), current_id_map.ToQuery(type->instantiations)));
    if (!type->uses.empty())
      types_uses.push_back(QueryType::UsesUpdate(current_id_map.ToQuery(type->id), current_id_map.ToQuery(type->uses)));
  },
    /*onFound:*/[this, &previous_id_map, &current_id_map](IndexedTypeDef* previous_def, IndexedTypeDef* current_def) {
    optional<QueryType::DefUpdate> previous_remapped_def = ToQuery(previous_id_map, previous_def->def);
    optional<QueryType::DefUpdate> current_remapped_def = ToQuery(current_id_map, current_def->def);
    if (current_remapped_def && previous_remapped_def != current_remapped_def)
      types_def_update.push_back(*current_remapped_def);

    PROCESS_UPDATE_DIFF(QueryTypeId, types_derived, derived, QueryTypeId);
    PROCESS_UPDATE_DIFF(QueryTypeId, types_instantiations, instantiations, QueryVarId);
    PROCESS_UPDATE_DIFF(QueryTypeId, types_uses, uses, QueryLocation);
  });

  // Functions
  CompareGroups<IndexedFuncDef>(previous_file.funcs, current_file.funcs,
    /*onRemoved:*/[this](IndexedFuncDef* def) {
    funcs_removed.push_back(def->def.usr);
  },
    /*onAdded:*/[this, &current_id_map](IndexedFuncDef* func) {
    if (!func->def.detailed_name.empty())
      funcs_def_update.push_back(ToQuery(current_id_map, func->def));
    if (!func->declarations.empty())
      funcs_declarations.push_back(QueryFunc::DeclarationsUpdate(current_id_map.ToQuery(func->id), current_id_map.ToQuery(func->declarations)));
    if (!func->derived.empty())
      funcs_derived.push_back(QueryFunc::DerivedUpdate(current_id_map.ToQuery(func->id), current_id_map.ToQuery(func->derived)));
    if (!func->callers.empty())
      funcs_callers.push_back(QueryFunc::CallersUpdate(current_id_map.ToQuery(func->id), current_id_map.ToQuery(func->callers)));
  },
    /*onFound:*/[this, &previous_id_map, &current_id_map](IndexedFuncDef* previous_def, IndexedFuncDef* current_def) {
    optional<QueryFunc::DefUpdate> previous_remapped_def = ToQuery(previous_id_map, previous_def->def);
    optional<QueryFunc::DefUpdate> current_remapped_def = ToQuery(current_id_map, current_def->def);
    if (current_remapped_def && previous_remapped_def != current_remapped_def)
      funcs_def_update.push_back(*current_remapped_def);

    PROCESS_UPDATE_DIFF(QueryFuncId, funcs_declarations, declarations, QueryLocation);
    PROCESS_UPDATE_DIFF(QueryFuncId, funcs_derived, derived, QueryFuncId);
    PROCESS_UPDATE_DIFF(QueryFuncId, funcs_callers, callers, QueryFuncRef);
  });

  // Variables
  CompareGroups<IndexedVarDef>(previous_file.vars, current_file.vars,
    /*onRemoved:*/[this](IndexedVarDef* def) {
    vars_removed.push_back(def->def.usr);
  },
    /*onAdded:*/[this, &current_id_map](IndexedVarDef* var) {
    if (!var->def.detailed_name.empty())
      vars_def_update.push_back(ToQuery(current_id_map, var->def));
    if (!var->uses.empty())
      vars_uses.push_back(QueryVar::UsesUpdate(current_id_map.ToQuery(var->id), current_id_map.ToQuery(var->uses)));
  },
    /*onFound:*/[this, &previous_id_map, &current_id_map](IndexedVarDef* previous_def, IndexedVarDef* current_def) {
    optional<QueryVar::DefUpdate> previous_remapped_def = ToQuery(previous_id_map, previous_def->def);
    optional<QueryVar::DefUpdate> current_remapped_def = ToQuery(current_id_map, current_def->def);
    if (current_remapped_def && previous_remapped_def != current_remapped_def)
      vars_def_update.push_back(*current_remapped_def);

    PROCESS_UPDATE_DIFF(QueryVarId, vars_uses, uses, QueryLocation);
  });

#undef PROCESS_UPDATE_DIFF
}

void IndexUpdate::Merge(const IndexUpdate& update) {
  // This function runs on an indexer thread.

#define INDEX_UPDATE_APPEND(name) \
    AddRange(&name, update.name);
#define INDEX_UPDATE_MERGE(name) \
    AddMergeableRange(&name, update.name);

  INDEX_UPDATE_APPEND(files_removed);
  INDEX_UPDATE_APPEND(files_def_update);

  INDEX_UPDATE_APPEND(types_removed);
  INDEX_UPDATE_APPEND(types_def_update);
  INDEX_UPDATE_MERGE(types_derived);
  INDEX_UPDATE_MERGE(types_instantiations);
  INDEX_UPDATE_MERGE(types_uses);

  INDEX_UPDATE_APPEND(funcs_removed);
  INDEX_UPDATE_APPEND(funcs_def_update);
  INDEX_UPDATE_MERGE(funcs_declarations);
  INDEX_UPDATE_MERGE(funcs_derived);
  INDEX_UPDATE_MERGE(funcs_callers);

  INDEX_UPDATE_APPEND(vars_removed);
  INDEX_UPDATE_APPEND(vars_def_update);
  INDEX_UPDATE_MERGE(vars_uses);

#undef INDEX_UPDATE_APPEND
#undef INDEX_UPDATE_MERGE
}





























// ------------------------
// QUERYDB THREAD FUNCTIONS
// ------------------------

void QueryDatabase::RemoveUsrs(const std::vector<Usr>& to_remove) {
  // This function runs on the querydb thread.

  // Actually removing data is extremely slow because every offset/index would
  // have to be updated. Instead, we just accept the memory overhead and mark
  // the symbol as invalid.
  //
  // If the user wants to reduce memory usage, they will have to restart the
  // indexer and load it from cache. Luckily, this doesn't take too long even
  // on large projects (1-2 minutes).

  for (Usr usr : to_remove)
    usr_to_symbol[usr].kind = SymbolKind::Invalid;
}

void QueryDatabase::ApplyIndexUpdate(IndexUpdate* update) {
  // This function runs on the querydb thread.

#define HANDLE_MERGEABLE(update_var_name, def_var_name, storage_name) \
  for (auto merge_update : update->update_var_name) { \
    auto* def = &storage_name[merge_update.id.id]; \
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

  RemoveUsrs(update->vars_removed);
  ImportOrUpdate(update->vars_def_update);
  HANDLE_MERGEABLE(vars_uses, uses, vars);

#undef HANDLE_MERGEABLE
}

void QueryDatabase::ImportOrUpdate(const std::vector<QueryFile::DefUpdate>& updates) {
  // This function runs on the querydb thread.

  for (auto& def : updates) {
    auto it = usr_to_symbol.find(def.path);
    assert(it != usr_to_symbol.end());

    QueryFile& existing = files[it->second.idx];
    existing.def = def;
    UpdateDetailedNames(&existing.detailed_name_idx, SymbolKind::File, it->second.idx, def.path);
  }
}

void QueryDatabase::ImportOrUpdate(const std::vector<QueryType::DefUpdate>& updates) {
  // This function runs on the querydb thread.

  for (auto& def : updates) {
    assert(!def.detailed_name.empty());

    auto it = usr_to_symbol.find(def.usr);
    assert(it != usr_to_symbol.end());

    QueryType& existing = types[it->second.idx];

    // Keep the existing definition if it is higher quality.
    if (existing.def.definition_spelling && !def.definition_spelling)
      continue;

    existing.def = def;
    UpdateDetailedNames(&existing.detailed_name_idx, SymbolKind::Type, it->second.idx, def.detailed_name);
  }
}

void QueryDatabase::ImportOrUpdate(const std::vector<QueryFunc::DefUpdate>& updates) {
  // This function runs on the querydb thread.

  for (auto& def : updates) {
    assert(!def.detailed_name.empty());

    auto it = usr_to_symbol.find(def.usr);
    assert(it != usr_to_symbol.end());

    QueryFunc& existing = funcs[it->second.idx];

    // Keep the existing definition if it is higher quality.
    if (existing.def.definition_spelling && !def.definition_spelling)
      continue;

    existing.def = def;
    UpdateDetailedNames(&existing.detailed_name_idx, SymbolKind::Func, it->second.idx, def.detailed_name);
  }
}

void QueryDatabase::ImportOrUpdate(const std::vector<QueryVar::DefUpdate>& updates) {
  // This function runs on the querydb thread.

  for (auto& def : updates) {
    assert(!def.detailed_name.empty());

    auto it = usr_to_symbol.find(def.usr);
    assert(it != usr_to_symbol.end());

    QueryVar& existing = vars[it->second.idx];

    // Keep the existing definition if it is higher quality.
    if (existing.def.definition_spelling && !def.definition_spelling)
      continue;

    existing.def = def;
    if (def.declaring_type)
      UpdateDetailedNames(&existing.detailed_name_idx, SymbolKind::Var, it->second.idx, def.detailed_name);
  }
}

void QueryDatabase::UpdateDetailedNames(size_t* qualified_name_index, SymbolKind kind, size_t symbol_index, const std::string& name) {
  if (*qualified_name_index == -1) {
    detailed_names.push_back(name);
    symbols.push_back(SymbolIdx(kind, symbol_index));
    *qualified_name_index = detailed_names.size() - 1;
  }
  else {
    detailed_names[*qualified_name_index] = name;
  }
}







// TODO: allow user to decide some indexer choices, ie, do we mark prototype parameters as usages?
