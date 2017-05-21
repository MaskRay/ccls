#include "query.h"

#include "indexer.h"

#include <optional.h>
#include <doctest/doctest.h>

#include <cassert>
#include <cstdint>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <iostream>

// TODO: Make all copy constructors explicit.


namespace {

QueryType::DefUpdate ToQuery(const IdMap& id_map, const IndexType::Def& type) {
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

QueryFunc::DefUpdate ToQuery(const IdMap& id_map, const IndexFunc::Def& func) {
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

QueryVar::DefUpdate ToQuery(const IdMap& id_map, const IndexVar::Def& var) {
  QueryVar::DefUpdate result(var.usr);
  result.short_name = var.short_name;
  result.detailed_name = var.detailed_name;
  result.declaration = id_map.ToQuery(var.declaration);
  result.definition_spelling = id_map.ToQuery(var.definition_spelling);
  result.definition_extent = id_map.ToQuery(var.definition_extent);
  result.variable_type = id_map.ToQuery(var.variable_type);
  result.declaring_type = id_map.ToQuery(var.declaring_type);
  result.is_local = var.is_local;
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

QueryFile::Def BuildFileDef(const IdMap& id_map, const IndexFile& indexed) {
  QueryFile::Def def;
  def.path = indexed.path;
  def.includes = indexed.includes;

  auto add_outline = [&def, &id_map](SymbolIdx idx, Range range) {
    def.outline.push_back(SymbolRef(idx, id_map.ToQuery(range)));
  };
  auto add_all_symbols = [&def, &id_map](SymbolIdx idx, Range range) {
    def.all_symbols.push_back(SymbolRef(idx, id_map.ToQuery(range)));
  };

  for (const IndexType& type : indexed.types) {
    if (type.def.definition_spelling.has_value())
      add_all_symbols(id_map.ToSymbol(type.id), type.def.definition_spelling.value());
    if (type.def.definition_extent.has_value())
      add_outline(id_map.ToSymbol(type.id), type.def.definition_extent.value());
    for (const Range& use : type.uses)
      add_all_symbols(id_map.ToSymbol(type.id), use);
  }
  for (const IndexFunc& func : indexed.funcs) {
    if (func.def.definition_spelling.has_value())
      add_all_symbols(id_map.ToSymbol(func.id), func.def.definition_spelling.value());
    if (func.def.definition_extent.has_value())
      add_outline(id_map.ToSymbol(func.id), func.def.definition_extent.value());
    for (Range decl : func.declarations) {
      add_all_symbols(id_map.ToSymbol(func.id), decl);
      add_outline(id_map.ToSymbol(func.id), decl);
    }
    for (const IndexFuncRef& caller : func.callers)
      add_all_symbols(id_map.ToSymbol(func.id), caller.loc);
  }
  for (const IndexVar& var : indexed.vars) {
    if (var.def.definition_spelling.has_value())
      add_all_symbols(id_map.ToSymbol(var.id), var.def.definition_spelling.value());
    if (var.def.definition_extent.has_value())
      add_outline(id_map.ToSymbol(var.id), var.def.definition_extent.value());
    for (const Range& use : var.uses)
      add_all_symbols(id_map.ToSymbol(var.id), use);
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
































QueryFileId GetQueryFileIdFromPath(QueryDatabase* query_db, const std::string& path) {
  auto it = query_db->usr_to_file.find(path);
  if (it != query_db->usr_to_file.end())
    return QueryFileId(it->second.id);

  size_t idx = query_db->files.size();
  query_db->usr_to_file[path] = QueryFileId(idx);
  query_db->files.push_back(QueryFile(path));
  return QueryFileId(idx);
}

QueryTypeId GetQueryTypeIdFromUsr(QueryDatabase* query_db, const Usr& usr) {
  auto it = query_db->usr_to_type.find(usr);
  if (it != query_db->usr_to_type.end())
    return QueryTypeId(it->second.id);

  size_t idx = query_db->types.size();
  query_db->usr_to_type[usr] = QueryTypeId(idx);
  query_db->types.push_back(QueryType(usr));
  return QueryTypeId(idx);
}

QueryFuncId GetQueryFuncIdFromUsr(QueryDatabase* query_db, const Usr& usr) {
  auto it = query_db->usr_to_func.find(usr);
  if (it != query_db->usr_to_func.end())
    return QueryFuncId(it->second.id);

  size_t idx = query_db->funcs.size();
  query_db->usr_to_func[usr] = QueryFuncId(idx);
  query_db->funcs.push_back(QueryFunc(usr));
  return QueryFuncId(idx);
}

QueryVarId GetQueryVarIdFromUsr(QueryDatabase* query_db, const Usr& usr) {
  auto it = query_db->usr_to_var.find(usr);
  if (it != query_db->usr_to_var.end())
    return QueryVarId(it->second.id);

  size_t idx = query_db->vars.size();
  query_db->usr_to_var[usr] = QueryVarId(idx);
  query_db->vars.push_back(QueryVar(usr));
  return QueryVarId(idx);
}

IdMap::IdMap(QueryDatabase* query_db, const IdCache& local_ids)
  : local_ids(local_ids) {
  primary_file = GetQueryFileIdFromPath(query_db, local_ids.primary_file);

  cached_type_ids_.resize(local_ids.type_id_to_usr.size());
  for (const auto& entry : local_ids.type_id_to_usr)
    cached_type_ids_[entry.first] = GetQueryTypeIdFromUsr(query_db, entry.second);

  cached_func_ids_.resize(local_ids.func_id_to_usr.size());
  for (const auto& entry : local_ids.func_id_to_usr)
    cached_func_ids_[entry.first] = GetQueryFuncIdFromUsr(query_db, entry.second);

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
  if (id.id == -1) return QueryFuncId((size_t)-1);
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
IndexUpdate IndexUpdate::CreateDelta(const IdMap* previous_id_map, const IdMap* current_id_map, IndexFile* previous, IndexFile* current) {
  // This function runs on an indexer thread.

  if (!previous_id_map) {
    assert(!previous);
    IndexFile previous(current->path);
    return IndexUpdate(*current_id_map, *current_id_map, previous, *current);
  }
  return IndexUpdate(*previous_id_map, *current_id_map, *previous, *current);
}

IndexUpdate::IndexUpdate(const IdMap& previous_id_map, const IdMap& current_id_map, IndexFile& previous_file, IndexFile& current_file) {
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
  CompareGroups<IndexType>(previous_file.types, current_file.types,
    /*onRemoved:*/[this](IndexType* def) {
    types_removed.push_back(def->def.usr);
  },
    /*onAdded:*/[this, &current_id_map](IndexType* type) {
    if (!type->def.detailed_name.empty())
      types_def_update.push_back(ToQuery(current_id_map, type->def));
    if (!type->derived.empty())
      types_derived.push_back(QueryType::DerivedUpdate(current_id_map.ToQuery(type->id), current_id_map.ToQuery(type->derived)));
    if (!type->instances.empty())
      types_instances.push_back(QueryType::InstancesUpdate(current_id_map.ToQuery(type->id), current_id_map.ToQuery(type->instances)));
    if (!type->uses.empty())
      types_uses.push_back(QueryType::UsesUpdate(current_id_map.ToQuery(type->id), current_id_map.ToQuery(type->uses)));
  },
    /*onFound:*/[this, &previous_id_map, &current_id_map](IndexType* previous_def, IndexType* current_def) {
    optional<QueryType::DefUpdate> previous_remapped_def = ToQuery(previous_id_map, previous_def->def);
    optional<QueryType::DefUpdate> current_remapped_def = ToQuery(current_id_map, current_def->def);
    if (current_remapped_def && previous_remapped_def != current_remapped_def && !current_remapped_def->detailed_name.empty())
      types_def_update.push_back(*current_remapped_def);

    PROCESS_UPDATE_DIFF(QueryTypeId, types_derived, derived, QueryTypeId);
    PROCESS_UPDATE_DIFF(QueryTypeId, types_instances, instances, QueryVarId);
    PROCESS_UPDATE_DIFF(QueryTypeId, types_uses, uses, QueryLocation);
  });

  // Functions
  CompareGroups<IndexFunc>(previous_file.funcs, current_file.funcs,
    /*onRemoved:*/[this](IndexFunc* def) {
    funcs_removed.push_back(def->def.usr);
  },
    /*onAdded:*/[this, &current_id_map](IndexFunc* func) {
    if (!func->def.detailed_name.empty())
      funcs_def_update.push_back(ToQuery(current_id_map, func->def));
    if (!func->declarations.empty())
      funcs_declarations.push_back(QueryFunc::DeclarationsUpdate(current_id_map.ToQuery(func->id), current_id_map.ToQuery(func->declarations)));
    if (!func->derived.empty())
      funcs_derived.push_back(QueryFunc::DerivedUpdate(current_id_map.ToQuery(func->id), current_id_map.ToQuery(func->derived)));
    if (!func->callers.empty())
      funcs_callers.push_back(QueryFunc::CallersUpdate(current_id_map.ToQuery(func->id), current_id_map.ToQuery(func->callers)));
  },
    /*onFound:*/[this, &previous_id_map, &current_id_map](IndexFunc* previous_def, IndexFunc* current_def) {
    optional<QueryFunc::DefUpdate> previous_remapped_def = ToQuery(previous_id_map, previous_def->def);
    optional<QueryFunc::DefUpdate> current_remapped_def = ToQuery(current_id_map, current_def->def);
    if (current_remapped_def && previous_remapped_def != current_remapped_def && !current_remapped_def->detailed_name.empty())
      funcs_def_update.push_back(*current_remapped_def);

    PROCESS_UPDATE_DIFF(QueryFuncId, funcs_declarations, declarations, QueryLocation);
    PROCESS_UPDATE_DIFF(QueryFuncId, funcs_derived, derived, QueryFuncId);
    PROCESS_UPDATE_DIFF(QueryFuncId, funcs_callers, callers, QueryFuncRef);
  });

  // Variables
  CompareGroups<IndexVar>(previous_file.vars, current_file.vars,
    /*onRemoved:*/[this](IndexVar* def) {
    vars_removed.push_back(def->def.usr);
  },
    /*onAdded:*/[this, &current_id_map](IndexVar* var) {
    if (!var->def.detailed_name.empty())
      vars_def_update.push_back(ToQuery(current_id_map, var->def));
    if (!var->uses.empty())
      vars_uses.push_back(QueryVar::UsesUpdate(current_id_map.ToQuery(var->id), current_id_map.ToQuery(var->uses)));
  },
    /*onFound:*/[this, &previous_id_map, &current_id_map](IndexVar* previous_def, IndexVar* current_def) {
    optional<QueryVar::DefUpdate> previous_remapped_def = ToQuery(previous_id_map, previous_def->def);
    optional<QueryVar::DefUpdate> current_remapped_def = ToQuery(current_id_map, current_def->def);
    if (current_remapped_def && previous_remapped_def != current_remapped_def && !current_remapped_def->detailed_name.empty())
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
  INDEX_UPDATE_MERGE(types_instances);
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

std::string IndexUpdate::ToString() {
  rapidjson::StringBuffer output;
  Writer writer(output);
  IndexUpdate& update = *this;
  Reflect(writer, update);
  return output.GetString();
}



























// ------------------------
// QUERYDB THREAD FUNCTIONS
// ------------------------

void QueryDatabase::RemoveUsrs(SymbolKind usr_kind, const std::vector<Usr>& to_remove) {
  // This function runs on the querydb thread.

  // When we remove an element, we just erase the state from the storage. We do
  // not update array indices because that would take a huge amount of time for
  // a very large index.
  //
  // There means that there is some memory growth that will never be reclaimed,
  // but it should be pretty minimal and is solved by simply restarting the
  // indexer and loading from cache, which is a fast operation.
  //
  // TODO: Add "cquery: Reload Index" command which unloads all querydb state
  // and fully reloads from cache. This will address the memory leak above.

  switch (usr_kind) {
    case SymbolKind::File: {
      for (const Usr& usr : to_remove)
        files[usr_to_file[usr].id] = nullopt;
      break;
    }
    case SymbolKind::Type: {
      for (const Usr& usr : to_remove)
        types[usr_to_type[usr].id] = nullopt;
      break;
    }
    case SymbolKind::Func: {
      for (const Usr& usr : to_remove)
        funcs[usr_to_func[usr].id] = nullopt;
      break;
    }
    case SymbolKind::Var: {
      for (const Usr& usr : to_remove)
        vars[usr_to_var[usr].id] = nullopt;
      break;
    }
    case SymbolKind::Invalid:
      break;
  }
}

void QueryDatabase::ApplyIndexUpdate(IndexUpdate* update) {
  // This function runs on the querydb thread.

  // Example types:
  //  storage_name       =>  std::vector<optional<QueryType>>
  //  merge_update       =>  QueryType::DerivedUpdate => MergeableUpdate<QueryTypeId, QueryTypeId>
  //  def                =>  QueryType
  //  def->def_var_name  =>  std::vector<QueryTypeId>
#define HANDLE_MERGEABLE(update_var_name, def_var_name, storage_name) \
  for (auto merge_update : update->update_var_name) { \
    auto& def = storage_name[merge_update.id.id]; \
    if (!def) \
      continue; /* TODO: Should we continue or create an empty def? */ \
    AddRange(&def->def_var_name, merge_update.to_add); \
    RemoveRange(&def->def_var_name, merge_update.to_remove); \
  }

  RemoveUsrs(SymbolKind::File, update->files_removed);
  ImportOrUpdate(update->files_def_update);

  RemoveUsrs(SymbolKind::Type, update->types_removed);
  ImportOrUpdate(update->types_def_update);
  HANDLE_MERGEABLE(types_derived, derived, types);
  HANDLE_MERGEABLE(types_instances, instances, types);
  HANDLE_MERGEABLE(types_uses, uses, types);

  RemoveUsrs(SymbolKind::Func, update->funcs_removed);
  ImportOrUpdate(update->funcs_def_update);
  HANDLE_MERGEABLE(funcs_declarations, declarations, funcs);
  HANDLE_MERGEABLE(funcs_derived, derived, funcs);
  HANDLE_MERGEABLE(funcs_callers, callers, funcs);

  RemoveUsrs(SymbolKind::Var, update->vars_removed);
  ImportOrUpdate(update->vars_def_update);
  HANDLE_MERGEABLE(vars_uses, uses, vars);

#undef HANDLE_MERGEABLE
}

void QueryDatabase::ImportOrUpdate(const std::vector<QueryFile::DefUpdate>& updates) {
  // This function runs on the querydb thread.

  for (auto& def : updates) {
    auto it = usr_to_file.find(def.path);
    assert(it != usr_to_file.end());

    optional<QueryFile>& existing = files[it->second.id];
    if (!existing)
      existing = QueryFile(def.path);

    existing->def = def;
    UpdateDetailedNames(&existing->detailed_name_idx, SymbolKind::File, it->second.id, def.path);
  }
}

void QueryDatabase::ImportOrUpdate(const std::vector<QueryType::DefUpdate>& updates) {
  // This function runs on the querydb thread.

  for (auto& def : updates) {
    assert(!def.detailed_name.empty());

    auto it = usr_to_type.find(def.usr);
    assert(it != usr_to_type.end());

    assert(it->second.id >= 0 && it->second.id < types.size());
    optional<QueryType>& existing = types[it->second.id];
    if (!existing)
      existing = QueryType(def.usr);

    // Keep the existing definition if it is higher quality.
    if (existing->def.definition_spelling && !def.definition_spelling)
      continue;

    existing->def = def;
    UpdateDetailedNames(&existing->detailed_name_idx, SymbolKind::Type, it->second.id, def.detailed_name);
  }
}

void QueryDatabase::ImportOrUpdate(const std::vector<QueryFunc::DefUpdate>& updates) {
  // This function runs on the querydb thread.

  for (auto& def : updates) {
    assert(!def.detailed_name.empty());

    auto it = usr_to_func.find(def.usr);
    assert(it != usr_to_func.end());

    assert(it->second.id >= 0 && it->second.id < funcs.size());
    optional<QueryFunc>& existing = funcs[it->second.id];
    if (!existing)
      existing = QueryFunc(def.usr);

    // Keep the existing definition if it is higher quality.
    if (existing->def.definition_spelling && !def.definition_spelling)
      continue;

    existing->def = def;
    UpdateDetailedNames(&existing->detailed_name_idx, SymbolKind::Func, it->second.id, def.detailed_name);
  }
}

void QueryDatabase::ImportOrUpdate(const std::vector<QueryVar::DefUpdate>& updates) {
  // This function runs on the querydb thread.

  for (auto& def : updates) {
    assert(!def.detailed_name.empty());

    auto it = usr_to_var.find(def.usr);
    assert(it != usr_to_var.end());

    assert(it->second.id >= 0 && it->second.id < vars.size());
    optional<QueryVar>& existing = vars[it->second.id];
    if (!existing)
      existing = QueryVar(def.usr);

    // Keep the existing definition if it is higher quality.
    if (existing->def.definition_spelling && !def.definition_spelling)
      continue;

    existing->def = def;
    if (def.declaring_type)
      UpdateDetailedNames(&existing->detailed_name_idx, SymbolKind::Var, it->second.id, def.detailed_name);
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

TEST_SUITE("query");

IndexUpdate GetDelta(IndexFile previous, IndexFile current) {
  QueryDatabase db;
  IdMap previous_map(&db, previous.id_cache);
  IdMap current_map(&db, current.id_cache);
  return IndexUpdate::CreateDelta(&previous_map, &current_map, &previous, &current);
}

TEST_CASE("remove defs") {
  IndexFile previous("foo.cc");
  IndexFile current("foo.cc");

  previous.ToTypeId("usr1");
  previous.ToFuncId("usr2");
  previous.ToVarId("usr3");

  IndexUpdate update = GetDelta(previous, current);

  REQUIRE(update.types_removed == std::vector<Usr>{ "usr1" });
  REQUIRE(update.funcs_removed == std::vector<Usr>{ "usr2" });
  REQUIRE(update.vars_removed == std::vector<Usr>{ "usr3" });
}

TEST_CASE("func callers") {
  IndexFile previous("foo.cc");
  IndexFile current("foo.cc");

  IndexFunc* pf = previous.Resolve(previous.ToFuncId("usr"));
  IndexFunc* cf = current.Resolve(current.ToFuncId("usr"));

  pf->callers.push_back(IndexFuncRef(IndexFuncId(0), Range(Position(1, 0))));
  cf->callers.push_back(IndexFuncRef(IndexFuncId(0), Range(Position(2, 0))));

  IndexUpdate update = GetDelta(previous, current);

  REQUIRE(update.funcs_removed == std::vector<Usr>{});
  REQUIRE(update.funcs_callers.size() == 1);
  REQUIRE(update.funcs_callers[0].id == QueryFuncId(0));
  REQUIRE(update.funcs_callers[0].to_remove.size() == 1);
  REQUIRE(update.funcs_callers[0].to_remove[0].loc.range == Range(Position(1, 0)));
  REQUIRE(update.funcs_callers[0].to_add.size() == 1);
  REQUIRE(update.funcs_callers[0].to_add[0].loc.range == Range(Position(2, 0)));
}

TEST_CASE("type usages") {
  IndexFile previous("foo.cc");
  IndexFile current("foo.cc");

  IndexType* pt = previous.Resolve(previous.ToTypeId("usr"));
  IndexType* ct = current.Resolve(current.ToTypeId("usr"));

  pt->uses.push_back(Range(Position(1, 0)));
  ct->uses.push_back(Range(Position(2, 0)));

  IndexUpdate update = GetDelta(previous, current);

  REQUIRE(update.types_removed == std::vector<Usr>{});
  REQUIRE(update.types_def_update == std::vector<QueryType::DefUpdate>{});
  REQUIRE(update.types_uses.size() == 1);
  REQUIRE(update.types_uses[0].to_remove.size() == 1);
  REQUIRE(update.types_uses[0].to_remove[0].range == Range(Position(1, 0)));
  REQUIRE(update.types_uses[0].to_add.size() == 1);
  REQUIRE(update.types_uses[0].to_add[0].range == Range(Position(2, 0)));
}

TEST_CASE("apply delta") {
  IndexFile previous("foo.cc");
  IndexFile current("foo.cc");

  IndexFunc* pf = previous.Resolve(previous.ToFuncId("usr"));
  IndexFunc* cf = current.Resolve(current.ToFuncId("usr"));
  pf->callers.push_back(IndexFuncRef(IndexFuncId(0), Range(Position(1, 0))));
  pf->callers.push_back(IndexFuncRef(IndexFuncId(0), Range(Position(2, 0))));
  cf->callers.push_back(IndexFuncRef(IndexFuncId(0), Range(Position(4, 0))));
  cf->callers.push_back(IndexFuncRef(IndexFuncId(0), Range(Position(5, 0))));

  QueryDatabase db;
  IdMap previous_map(&db, previous.id_cache);
  IdMap current_map(&db, current.id_cache);
  REQUIRE(db.funcs.size() == 1);

  IndexUpdate import_update = IndexUpdate::CreateDelta(nullptr, &previous_map, nullptr, &previous);
  IndexUpdate delta_update = IndexUpdate::CreateDelta(&previous_map, &current_map, &previous, &current);

  db.ApplyIndexUpdate(&import_update);
  REQUIRE(db.funcs[0]->callers.size() == 2);
  REQUIRE(db.funcs[0]->callers[0].loc.range == Range(Position(1, 0)));
  REQUIRE(db.funcs[0]->callers[1].loc.range == Range(Position(2, 0)));

  db.ApplyIndexUpdate(&delta_update);
  REQUIRE(db.funcs[0]->callers.size() == 2);
  REQUIRE(db.funcs[0]->callers[0].loc.range == Range(Position(4, 0)));
  REQUIRE(db.funcs[0]->callers[1].loc.range == Range(Position(5, 0)));
}

TEST_SUITE_END();