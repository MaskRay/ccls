#include "query.h"

#include "indexer.h"
#include "serializer.h"
#include "serializers/json.h"

#include <doctest/doctest.h>
#include <optional.h>
#include <loguru.hpp>

#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

// TODO: Make all copy constructors explicit.

namespace {

template <typename T>
void VerifyUnique(const std::vector<T>& values0) {
// FIXME: Run on a big code-base for a while and verify no assertions are
// triggered.
#if false
  auto values = values0;
  std::sort(values.begin(), values.end());
  assert(std::unique(values.begin(), values.end()) == values.end());
#endif
}

template <typename T>
void RemoveRange(std::vector<T>* dest, const std::vector<T>& to_remove) {
  std::unordered_set<T> to_remove_set(to_remove.begin(), to_remove.end());
  dest->erase(
      std::remove_if(dest->begin(), dest->end(),
                     [&](const T& t) { return to_remove_set.count(t) > 0; }),
      dest->end());
}

optional<QueryType::Def> ToQuery(const IdMap& id_map,
                                 const IndexType::Def& type) {
  if (type.detailed_name.empty())
    return nullopt;

  QueryType::Def result;
  result.detailed_name = type.detailed_name;
  result.short_name_offset = type.short_name_offset;
  result.short_name_size = type.short_name_size;
  result.kind = type.kind;
  if (!type.hover.empty())
    result.hover = type.hover;
  if (!type.comments.empty())
    result.comments = type.comments;
  result.file = id_map.primary_file;
  result.spell = id_map.ToQuery(type.spell);
  result.extent = id_map.ToQuery(type.extent);
  result.alias_of = id_map.ToQuery(type.alias_of);
  result.bases = id_map.ToQuery(type.bases);
  result.types = id_map.ToQuery(type.types);
  result.funcs = id_map.ToQuery(type.funcs);
  result.vars = id_map.ToQuery(type.vars);
  return result;
}

optional<QueryFunc::Def> ToQuery(const IdMap& id_map,
                                 const IndexFunc::Def& func) {
  if (func.detailed_name.empty())
    return nullopt;

  QueryFunc::Def result;
  result.detailed_name = func.detailed_name;
  result.short_name_offset = func.short_name_offset;
  result.short_name_size = func.short_name_size;
  result.kind = func.kind;
  result.storage = func.storage;
  if (!func.hover.empty())
    result.hover = func.hover;
  if (!func.comments.empty())
    result.comments = func.comments;
  result.file = id_map.primary_file;
  result.spell = id_map.ToQuery(func.spell);
  result.extent = id_map.ToQuery(func.extent);
  result.declaring_type = id_map.ToQuery(func.declaring_type);
  result.bases = id_map.ToQuery(func.bases);
  result.vars = id_map.ToQuery(func.vars);
  result.callees = id_map.ToQuery(func.callees);
  return result;
}

optional<QueryVar::Def> ToQuery(const IdMap& id_map, const IndexVar::Def& var) {
  if (var.detailed_name.empty())
    return nullopt;

  QueryVar::Def result;
  result.detailed_name = var.detailed_name;
  result.short_name_offset = var.short_name_offset;
  result.short_name_size = var.short_name_size;
  if (!var.hover.empty())
    result.hover = var.hover;
  if (!var.comments.empty())
    result.comments = var.comments;
  result.file = id_map.primary_file;
  result.spell = id_map.ToQuery(var.spell);
  result.extent = id_map.ToQuery(var.extent);
  result.type = id_map.ToQuery(var.type);
  result.kind = var.kind;
  result.storage = var.storage;
  return result;
}

// Adds the mergeable updates in |source| to |dest|. If a mergeable update for
// the destination type already exists, it will be combined. This makes merging
// updates take longer but reduces import time on the querydb thread.
template <typename TId, typename TValue>
void AddMergeableRange(std::vector<MergeableUpdate<TId, TValue>>* dest,
                       std::vector<MergeableUpdate<TId, TValue>>&& source) {
  // TODO: Consider caching the lookup table. It can probably save even more
  // time at the cost of some additional memory.

  // Build lookup table.
  spp::sparse_hash_map<TId, size_t> id_to_index;
  id_to_index.resize(dest->size());
  for (size_t i = 0; i < dest->size(); ++i)
    id_to_index[(*dest)[i].id] = i;

  // Add entries. Try to add them to an existing entry.
  for (auto& entry : source) {
    auto it = id_to_index.find(entry.id);
    if (it != id_to_index.end()) {
      AddRange(&(*dest)[it->second].to_add, std::move(entry.to_add));
      AddRange(&(*dest)[it->second].to_remove, std::move(entry.to_remove));
    } else {
      dest->push_back(std::move(entry));
    }
  }
}

// Compares |previous| and |current|, adding all elements that are in |previous|
// but not |current| to |removed|, and all elements that are in |current| but
// not |previous| to |added|.
//
// Returns true iff |removed| or |added| are non-empty.
template <typename T>
bool ComputeDifferenceForUpdate(std::vector<T>&& previous,
                                std::vector<T>&& current,
                                std::vector<T>* removed,
                                std::vector<T>* added) {
  // We need to sort to use std::set_difference.
  std::sort(previous.begin(), previous.end());
  std::sort(current.begin(), current.end());

  auto it0 = previous.begin(), it1 = current.begin();
  while (it0 != previous.end() && it1 != current.end()) {
    // Elements in |previous| that are not in |current|.
    if (*it0 < *it1)
      removed->push_back(std::move(*it0++));
    // Elements in |current| that are not in |previous|.
    else if (*it1 < *it0)
      added->push_back(std::move(*it1++));
    else
      ++it0, ++it1;
  }
  while (it0 != previous.end())
    removed->push_back(std::move(*it0++));
  while (it1 != current.end())
    added->push_back(std::move(*it1++));

  return !removed->empty() || !added->empty();
}

template <typename T>
void CompareGroups(std::vector<T>& previous_data,
                   std::vector<T>& current_data,
                   std::function<void(T*)> on_removed,
                   std::function<void(T*)> on_added,
                   std::function<void(T*, T*)> on_found) {
  std::sort(previous_data.begin(), previous_data.end());
  std::sort(current_data.begin(), current_data.end());

  auto prev_it = previous_data.begin();
  auto curr_it = current_data.begin();
  while (prev_it != previous_data.end() && curr_it != current_data.end()) {
    // same id
    if (prev_it->usr == curr_it->usr) {
      on_found(&*prev_it, &*curr_it);
      ++prev_it;
      ++curr_it;
    }

    // prev_id is smaller - prev_it has data curr_it does not have.
    else if (prev_it->usr < curr_it->usr) {
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

QueryFile::DefUpdate BuildFileDefUpdate(const IdMap& id_map,
                                        const IndexFile& indexed) {
  QueryFile::Def def;
  def.file = id_map.primary_file;
  def.path = indexed.path;
  def.args = indexed.args;
  def.includes = indexed.includes;
  def.inactive_regions = indexed.skipped_by_preprocessor;
  def.dependencies = indexed.dependencies;

  // Convert enum to markdown compatible strings
  def.language = [&indexed]() {
    switch (indexed.language) {
      case LanguageId::C:
        return "c";
      case LanguageId::Cpp:
        return "cpp";
      case LanguageId::ObjC:
        return "objective-c";
      case LanguageId::ObjCpp:
        return "objective-cpp";
      default:
        return "";
    }
  }();

  auto add_all_symbols = [&](Use use, Id<void> id, SymbolKind kind) {
    def.all_symbols.push_back(SymbolRef(use.range, id, kind, use.role));
  };
  auto add_outline = [&](Use use, Id<void> id, SymbolKind kind) {
    def.outline.push_back(SymbolRef(use.range, id, kind, use.role));
  };

  for (const IndexType& type : indexed.types) {
    QueryTypeId id = id_map.ToQuery(type.id);
    if (type.def.spell)
      add_all_symbols(*type.def.spell, id, SymbolKind::Type);
    if (type.def.extent)
      add_outline(*type.def.extent, id, SymbolKind::Type);
    for (Use decl : type.declarations) {
      add_all_symbols(decl, id, SymbolKind::Type);
      // Constructor positions have references to the class,
      // which we do not want to show in textDocument/documentSymbol
      if (!(decl.role & Role::Reference))
        add_outline(decl, id, SymbolKind::Type);
    }
    for (Use use : type.uses)
      add_all_symbols(use, id, SymbolKind::Type);
  }
  for (const IndexFunc& func : indexed.funcs) {
    QueryFuncId id = id_map.ToQuery(func.id);
    if (func.def.spell)
      add_all_symbols(*func.def.spell, id, SymbolKind::Func);
    if (func.def.extent)
      add_outline(*func.def.extent, id, SymbolKind::Func);
    for (const IndexFunc::Declaration& decl : func.declarations) {
      add_all_symbols(decl.spell, id, SymbolKind::Func);
      add_outline(decl.spell, id, SymbolKind::Func);
    }
    for (Use use : func.uses) {
      // Make ranges of implicit function calls larger (spanning one more column
      // to the left/right). This is hacky but useful. e.g.
      // textDocument/definition on the space/semicolon in `A a;` or `return
      // 42;` will take you to the constructor.
      if (use.role & Role::Implicit) {
        if (use.range.start.column > 0)
          use.range.start.column--;
        use.range.end.column++;
      }
      add_all_symbols(use, id, SymbolKind::Func);
    }
  }
  for (const IndexVar& var : indexed.vars) {
    QueryVarId id = id_map.ToQuery(var.id);
    if (var.def.spell)
      add_all_symbols(*var.def.spell, id, SymbolKind::Var);
    if (var.def.extent)
      add_outline(*var.def.extent, id, SymbolKind::Var);
    for (Use decl : var.declarations) {
      add_all_symbols(decl, id, SymbolKind::Var);
      add_outline(decl, id, SymbolKind::Var);
    }
    for (Use use : var.uses)
      add_all_symbols(use, id, SymbolKind::Var);
  }

  std::sort(def.outline.begin(), def.outline.end(),
            [](const SymbolRef& a, const SymbolRef& b) {
              return a.range.start < b.range.start;
            });
  std::sort(def.all_symbols.begin(), def.all_symbols.end(),
            [](const SymbolRef& a, const SymbolRef& b) {
              return a.range.start < b.range.start;
            });

  return QueryFile::DefUpdate(def, indexed.file_contents);
}

Maybe<QueryFileId> GetQueryFileIdFromPath(QueryDatabase* query_db,
                                          const std::string& path,
                                          bool create_if_missing) {
  NormalizedPath normalized_path(path);
  auto it = query_db->usr_to_file.find(normalized_path);
  if (it != query_db->usr_to_file.end())
    return QueryFileId(it->second.id);
  if (!create_if_missing)
    return {};

  RawId idx = query_db->files.size();
  query_db->usr_to_file[normalized_path] = QueryFileId(idx);
  query_db->files.push_back(QueryFile(path));
  return QueryFileId(idx);
}

Maybe<QueryTypeId> GetQueryTypeIdFromUsr(QueryDatabase* query_db,
                                         Usr usr,
                                         bool create_if_missing) {
  auto it = query_db->usr_to_type.find(usr);
  if (it != query_db->usr_to_type.end())
    return QueryTypeId(it->second.id);
  if (!create_if_missing)
    return {};

  RawId idx = query_db->types.size();
  query_db->usr_to_type[usr] = QueryTypeId(idx);
  query_db->types.push_back(QueryType(usr));
  return QueryTypeId(idx);
}

Maybe<QueryFuncId> GetQueryFuncIdFromUsr(QueryDatabase* query_db,
                                         Usr usr,
                                         bool create_if_missing) {
  auto it = query_db->usr_to_func.find(usr);
  if (it != query_db->usr_to_func.end())
    return QueryFuncId(it->second.id);
  if (!create_if_missing)
    return {};

  RawId idx = query_db->funcs.size();
  query_db->usr_to_func[usr] = QueryFuncId(idx);
  query_db->funcs.push_back(QueryFunc(usr));
  return QueryFuncId(idx);
}

Maybe<QueryVarId> GetQueryVarIdFromUsr(QueryDatabase* query_db,
                                       Usr usr,
                                       bool create_if_missing) {
  auto it = query_db->usr_to_var.find(usr);
  if (it != query_db->usr_to_var.end())
    return QueryVarId(it->second.id);
  if (!create_if_missing)
    return {};

  RawId idx = query_db->vars.size();
  query_db->usr_to_var[usr] = QueryVarId(idx);
  query_db->vars.push_back(QueryVar(usr));
  return QueryVarId(idx);
}

// Returns true if an element with the same file is found.
template <typename Q>
bool TryReplaceDef(std::forward_list<Q>& def_list, Q&& def) {
  for (auto& def1 : def_list)
    if (def1.file == def.file) {
      if (!def1.spell || def.spell)
        def1 = std::move(def);
      return true;
    }
  return false;
}

}  // namespace

Maybe<QueryFileId> QueryDatabase::GetQueryFileIdFromPath(
    const std::string& path) {
  return ::GetQueryFileIdFromPath(this, path, false);
}

Maybe<QueryTypeId> QueryDatabase::GetQueryTypeIdFromUsr(Usr usr) {
  return ::GetQueryTypeIdFromUsr(this, usr, false);
}

Maybe<QueryFuncId> QueryDatabase::GetQueryFuncIdFromUsr(Usr usr) {
  return ::GetQueryFuncIdFromUsr(this, usr, false);
}

Maybe<QueryVarId> QueryDatabase::GetQueryVarIdFromUsr(Usr usr) {
  return ::GetQueryVarIdFromUsr(this, usr, false);
}

IdMap::IdMap(QueryDatabase* query_db, const IdCache& local_ids)
    : local_ids(local_ids) {
  // LOG_S(INFO) << "Creating IdMap for " << local_ids.primary_file;
  primary_file =
      *GetQueryFileIdFromPath(query_db, local_ids.primary_file, true);

  cached_type_ids_.resize(local_ids.type_id_to_usr.size());
  for (const auto& entry : local_ids.type_id_to_usr)
    cached_type_ids_[entry.first] =
        *GetQueryTypeIdFromUsr(query_db, entry.second, true);

  cached_func_ids_.resize(local_ids.func_id_to_usr.size());
  for (const auto& entry : local_ids.func_id_to_usr)
    cached_func_ids_[entry.first] =
        *GetQueryFuncIdFromUsr(query_db, entry.second, true);

  cached_var_ids_.resize(local_ids.var_id_to_usr.size());
  for (const auto& entry : local_ids.var_id_to_usr)
    cached_var_ids_[entry.first] =
        *GetQueryVarIdFromUsr(query_db, entry.second, true);
}

QueryTypeId IdMap::ToQuery(IndexTypeId id) const {
  assert(cached_type_ids_.find(id) != cached_type_ids_.end());
  return QueryTypeId(cached_type_ids_.find(id)->second);
}
QueryFuncId IdMap::ToQuery(IndexFuncId id) const {
  assert(cached_func_ids_.find(id) != cached_func_ids_.end());
  return QueryFuncId(cached_func_ids_.find(id)->second);
}
QueryVarId IdMap::ToQuery(IndexVarId id) const {
  assert(cached_var_ids_.find(id) != cached_var_ids_.end());
  return QueryVarId(cached_var_ids_.find(id)->second);
}

Use IdMap::ToQuery(Reference ref) const {
  Use ret(ref.range, ref.id, ref.kind, ref.role, primary_file);
  switch (ref.kind) {
    case SymbolKind::Invalid:
      break;
    case SymbolKind::File:
      ret.id = primary_file;
      break;
    case SymbolKind::Func:
      ret.id = ToQuery(IndexFuncId(ref.id));
      break;
    case SymbolKind::Type:
      ret.id = ToQuery(IndexTypeId(ref.id));
      break;
    case SymbolKind::Var:
      ret.id = ToQuery(IndexVarId(ref.id));
      break;
  }
  return ret;
}
SymbolRef IdMap::ToQuery(SymbolRef ref) const {
  ref.Reference::operator=(ToQuery(static_cast<Reference>(ref)));
  return ref;
}
Use IdMap::ToQuery(Use use) const {
  return ToQuery(static_cast<Reference>(use));
}

Use IdMap::ToQuery(IndexFunc::Declaration decl) const {
  return ToQuery(static_cast<Reference>(decl.spell));
}

// ----------------------
// INDEX THREAD FUNCTIONS
// ----------------------

// static
IndexUpdate IndexUpdate::CreateDelta(const IdMap* previous_id_map,
                                     const IdMap* current_id_map,
                                     IndexFile* previous,
                                     IndexFile* current) {
  // This function runs on an indexer thread.

  if (!previous_id_map) {
    assert(!previous);
    IndexFile empty(current->path, "<empty>");
    return IndexUpdate(*current_id_map, *current_id_map, empty, *current);
  }
  return IndexUpdate(*previous_id_map, *current_id_map, *previous, *current);
}

IndexUpdate::IndexUpdate(const IdMap& previous_id_map,
                         const IdMap& current_id_map,
                         IndexFile& previous_file,
                         IndexFile& current_file) {
// This function runs on an indexer thread.

// |query_name| is the name of the variable on the query type.
// |index_name| is the name of the variable on the index type.
// |type| is the type of the variable.
#define PROCESS_UPDATE_DIFF(type_id, query_name, index_name, type)       \
  {                                                                      \
    /* Check for changes. */                                             \
    std::vector<type> removed, added;                                    \
    auto query_previous = previous_id_map.ToQuery(previous->index_name); \
    auto query_current = current_id_map.ToQuery(current->index_name);    \
    bool did_add = ComputeDifferenceForUpdate(std::move(query_previous), \
                                              std::move(query_current),  \
                                              &removed, &added);         \
    if (did_add) {                                                       \
      query_name.push_back(MergeableUpdate<type_id, type>(               \
          current_id_map.ToQuery(current->id), std::move(added),         \
          std::move(removed)));                                          \
    }                                                                    \
  }
  // File
  files_def_update.push_back(BuildFileDefUpdate(current_id_map, current_file));

  // **NOTE** We only remove entries if they were defined in the previous index.
  // For example, if a type is included from another file it will be defined
  // simply so we can attribute the usage/reference to it. If the reference goes
  // away we don't want to remove the type/func/var usage.

  // Types
  CompareGroups<IndexType>(
      previous_file.types, current_file.types,
      /*onRemoved:*/
      [this, &previous_id_map](IndexType* type) {
        if (type->def.spell)
          types_removed.push_back(type->usr);
        if (!type->declarations.empty())
          types_declarations.push_back(QueryType::DeclarationsUpdate(
              previous_id_map.ToQuery(type->id), {},
              previous_id_map.ToQuery(type->declarations)));
        if (!type->derived.empty())
          types_derived.push_back(
              QueryType::DerivedUpdate(previous_id_map.ToQuery(type->id), {},
                                       previous_id_map.ToQuery(type->derived)));
        if (!type->instances.empty())
          types_instances.push_back(QueryType::InstancesUpdate(
              previous_id_map.ToQuery(type->id), {},
              previous_id_map.ToQuery(type->instances)));
        if (!type->uses.empty())
          types_uses.push_back(
              QueryType::UsesUpdate(previous_id_map.ToQuery(type->id), {},
                                    previous_id_map.ToQuery(type->uses)));
      },
      /*onAdded:*/
      [this, &current_id_map](IndexType* type) {
        optional<QueryType::Def> def_update =
            ToQuery(current_id_map, type->def);
        if (def_update)
          types_def_update.push_back(
              QueryType::DefUpdate(type->usr, std::move(*def_update)));
        if (!type->declarations.empty())
          types_declarations.push_back(QueryType::DeclarationsUpdate(
              current_id_map.ToQuery(type->id),
              current_id_map.ToQuery(type->declarations)));
        if (!type->derived.empty())
          types_derived.push_back(
              QueryType::DerivedUpdate(current_id_map.ToQuery(type->id),
                                       current_id_map.ToQuery(type->derived)));
        if (!type->instances.empty())
          types_instances.push_back(QueryType::InstancesUpdate(
              current_id_map.ToQuery(type->id),
              current_id_map.ToQuery(type->instances)));
        if (!type->uses.empty())
          types_uses.push_back(
              QueryType::UsesUpdate(current_id_map.ToQuery(type->id),
                                    current_id_map.ToQuery(type->uses)));
      },
      /*onFound:*/
      [this, &previous_id_map, &current_id_map](IndexType* previous,
                                                IndexType* current) {
        optional<QueryType::Def> previous_remapped_def =
            ToQuery(previous_id_map, previous->def);
        optional<QueryType::Def> current_remapped_def =
            ToQuery(current_id_map, current->def);
        if (current_remapped_def &&
            previous_remapped_def != current_remapped_def &&
            !current_remapped_def->detailed_name.empty()) {
          types_def_update.push_back(QueryType::DefUpdate(
              current->usr, std::move(*current_remapped_def)));
        }

        PROCESS_UPDATE_DIFF(QueryTypeId, types_declarations, declarations, Use);
        PROCESS_UPDATE_DIFF(QueryTypeId, types_derived, derived, QueryTypeId);
        PROCESS_UPDATE_DIFF(QueryTypeId, types_instances, instances,
                            QueryVarId);
        PROCESS_UPDATE_DIFF(QueryTypeId, types_uses, uses, Use);
      });

  // Functions
  CompareGroups<IndexFunc>(
      previous_file.funcs, current_file.funcs,
      /*onRemoved:*/
      [this, &previous_id_map](IndexFunc* func) {
        if (func->def.spell)
          funcs_removed.emplace_back(func->usr, previous_id_map.primary_file);
        if (!func->declarations.empty())
          funcs_declarations.push_back(QueryFunc::DeclarationsUpdate(
              previous_id_map.ToQuery(func->id), {},
              previous_id_map.ToQuery(func->declarations)));
        if (!func->derived.empty())
          funcs_derived.push_back(
              QueryFunc::DerivedUpdate(previous_id_map.ToQuery(func->id), {},
                                       previous_id_map.ToQuery(func->derived)));
        if (!func->uses.empty())
          funcs_uses.push_back(
              QueryFunc::UsesUpdate(previous_id_map.ToQuery(func->id), {},
                                    previous_id_map.ToQuery(func->uses)));
      },
      /*onAdded:*/
      [this, &current_id_map](IndexFunc* func) {
        optional<QueryFunc::Def> def_update =
            ToQuery(current_id_map, func->def);
        if (def_update)
          funcs_def_update.push_back(
              QueryFunc::DefUpdate(func->usr, std::move(*def_update)));
        if (!func->declarations.empty())
          funcs_declarations.push_back(QueryFunc::DeclarationsUpdate(
              current_id_map.ToQuery(func->id),
              current_id_map.ToQuery(func->declarations)));
        if (!func->derived.empty())
          funcs_derived.push_back(
              QueryFunc::DerivedUpdate(current_id_map.ToQuery(func->id),
                                       current_id_map.ToQuery(func->derived)));
        if (!func->uses.empty())
          funcs_uses.push_back(
              QueryFunc::UsesUpdate(current_id_map.ToQuery(func->id),
                                    current_id_map.ToQuery(func->uses)));
      },
      /*onFound:*/
      [this, &previous_id_map, &current_id_map](IndexFunc* previous,
                                                IndexFunc* current) {
        optional<QueryFunc::Def> previous_remapped_def =
            ToQuery(previous_id_map, previous->def);
        optional<QueryFunc::Def> current_remapped_def =
            ToQuery(current_id_map, current->def);
        if (current_remapped_def &&
            previous_remapped_def != current_remapped_def &&
            !current_remapped_def->detailed_name.empty()) {
          funcs_def_update.push_back(QueryFunc::DefUpdate(
              current->usr, std::move(*current_remapped_def)));
        }

        PROCESS_UPDATE_DIFF(QueryFuncId, funcs_declarations, declarations, Use);
        PROCESS_UPDATE_DIFF(QueryFuncId, funcs_derived, derived, QueryFuncId);
        PROCESS_UPDATE_DIFF(QueryFuncId, funcs_uses, uses, Use);
      });

  // Variables
  CompareGroups<IndexVar>(
      previous_file.vars, current_file.vars,
      /*onRemoved:*/
      [this, &previous_id_map](IndexVar* var) {
        if (var->def.spell)
          vars_removed.emplace_back(var->usr, previous_id_map.primary_file);
        if (!var->declarations.empty())
          vars_declarations.push_back(QueryVar::DeclarationsUpdate(
              previous_id_map.ToQuery(var->id), {},
              previous_id_map.ToQuery(var->declarations)));
        if (!var->uses.empty())
          vars_uses.push_back(
              QueryVar::UsesUpdate(previous_id_map.ToQuery(var->id), {},
                                   previous_id_map.ToQuery(var->uses)));
      },
      /*onAdded:*/
      [this, &current_id_map](IndexVar* var) {
        optional<QueryVar::Def> def_update = ToQuery(current_id_map, var->def);
        if (def_update)
          vars_def_update.push_back(
              QueryVar::DefUpdate(var->usr, std::move(*def_update)));
        if (!var->declarations.empty())
          vars_declarations.push_back(QueryVar::DeclarationsUpdate(
              current_id_map.ToQuery(var->id),
              current_id_map.ToQuery(var->declarations)));
        if (!var->uses.empty())
          vars_uses.push_back(
              QueryVar::UsesUpdate(current_id_map.ToQuery(var->id),
                                   current_id_map.ToQuery(var->uses)));
      },
      /*onFound:*/
      [this, &previous_id_map, &current_id_map](IndexVar* previous,
                                                IndexVar* current) {
        optional<QueryVar::Def> previous_remapped_def =
            ToQuery(previous_id_map, previous->def);
        optional<QueryVar::Def> current_remapped_def =
            ToQuery(current_id_map, current->def);
        if (current_remapped_def &&
            previous_remapped_def != current_remapped_def &&
            !current_remapped_def->detailed_name.empty())
          vars_def_update.push_back(QueryVar::DefUpdate(
              current->usr, std::move(*current_remapped_def)));

        PROCESS_UPDATE_DIFF(QueryVarId, vars_declarations, declarations, Use);
        PROCESS_UPDATE_DIFF(QueryVarId, vars_uses, uses, Use);
      });

#undef PROCESS_UPDATE_DIFF
}

// This function runs on an indexer thread.
void IndexUpdate::Merge(IndexUpdate&& update) {
#define INDEX_UPDATE_APPEND(name) AddRange(&name, std::move(update.name));
#define INDEX_UPDATE_MERGE(name) \
  AddMergeableRange(&name, std::move(update.name));

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
  INDEX_UPDATE_MERGE(funcs_uses);

  INDEX_UPDATE_APPEND(vars_removed);
  INDEX_UPDATE_APPEND(vars_def_update);
  INDEX_UPDATE_MERGE(vars_declarations);
  INDEX_UPDATE_MERGE(vars_uses);

#undef INDEX_UPDATE_APPEND
#undef INDEX_UPDATE_MERGE
}

std::string IndexUpdate::ToString() {
  rapidjson::StringBuffer output;
  rapidjson::Writer<rapidjson::StringBuffer> writer(output);
  JsonWriter json_writer(&writer);
  IndexUpdate& update = *this;
  Reflect(json_writer, update);
  return output.GetString();
}

NormalizedPath::NormalizedPath(const std::string& path)
    : path(LowerPathIfCaseInsensitive(path)) {}

bool NormalizedPath::operator==(const NormalizedPath& rhs) const {
  return path == rhs.path;
}

bool NormalizedPath::operator!=(const NormalizedPath& rhs) const {
  return path != rhs.path;
}

// ------------------------
// QUERYDB THREAD FUNCTIONS
// ------------------------

void QueryDatabase::RemoveUsrs(SymbolKind usr_kind,
                               const std::vector<Usr>& to_remove) {
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
    case SymbolKind::Type: {
      for (const Usr& usr : to_remove) {
        QueryType& type = types[usr_to_type[usr].id];
        if (type.symbol_idx)
          symbols[type.symbol_idx->id].kind = SymbolKind::Invalid;
        type.def.clear();
      }
      break;
    }
    default:
      break;
  }
}

void QueryDatabase::RemoveUsrs(
    SymbolKind usr_kind,
    const std::vector<WithUsr<QueryFileId>>& to_remove) {
  switch (usr_kind) {
    case SymbolKind::Func: {
      for (const auto& usr_file : to_remove) {
        QueryFunc& func = funcs[usr_to_func[usr_file.usr].id];
        func.def.remove_if([&](const QueryFunc::Def& def) {
          return def.file == usr_file.value;
        });
      }
      break;
    }
    case SymbolKind::Var: {
      for (const auto& usr_file : to_remove) {
        QueryVar& var = vars[usr_to_var[usr_file.usr].id];
        var.def.remove_if([&](const QueryVar::Def& def) {
          return def.file == usr_file.value;
        });
      }
      break;
    }
    default:
      assert(false);
      break;
  }
}

void QueryDatabase::ApplyIndexUpdate(IndexUpdate* update) {
// This function runs on the querydb thread.

// Example types:
//  storage_name       =>  std::vector<optional<QueryType>>
//  merge_update       =>  QueryType::DerivedUpdate =>
//  MergeableUpdate<QueryTypeId, QueryTypeId> def                =>  QueryType
//  def->def_var_name  =>  std::vector<QueryTypeId>
#define HANDLE_MERGEABLE(update_var_name, def_var_name, storage_name) \
  for (auto merge_update : update->update_var_name) {                 \
    auto& def = storage_name[merge_update.id.id];                     \
    AddRange(&def.def_var_name, merge_update.to_add);                 \
    RemoveRange(&def.def_var_name, merge_update.to_remove);           \
    VerifyUnique(def.def_var_name);                                   \
  }

  for (const std::string& filename : update->files_removed)
    files[usr_to_file[NormalizedPath(filename)].id].def = nullopt;
  ImportOrUpdate(update->files_def_update);

  RemoveUsrs(SymbolKind::Type, update->types_removed);
  ImportOrUpdate(std::move(update->types_def_update));
  HANDLE_MERGEABLE(types_declarations, declarations, types);
  HANDLE_MERGEABLE(types_derived, derived, types);
  HANDLE_MERGEABLE(types_instances, instances, types);
  HANDLE_MERGEABLE(types_uses, uses, types);

  RemoveUsrs(SymbolKind::Func, update->funcs_removed);
  ImportOrUpdate(std::move(update->funcs_def_update));
  HANDLE_MERGEABLE(funcs_declarations, declarations, funcs);
  HANDLE_MERGEABLE(funcs_derived, derived, funcs);
  HANDLE_MERGEABLE(funcs_uses, uses, funcs);

  RemoveUsrs(SymbolKind::Var, update->vars_removed);
  ImportOrUpdate(std::move(update->vars_def_update));
  HANDLE_MERGEABLE(vars_declarations, declarations, vars);
  HANDLE_MERGEABLE(vars_uses, uses, vars);

#undef HANDLE_MERGEABLE
}

void QueryDatabase::ImportOrUpdate(
    const std::vector<QueryFile::DefUpdate>& updates) {
  // This function runs on the querydb thread.

  for (auto& def : updates) {
    auto it = usr_to_file.find(NormalizedPath(def.value.path));
    assert(it != usr_to_file.end());

    QueryFile& existing = files[it->second.id];

    existing.def = def.value;
    UpdateSymbols(&existing.symbol_idx, SymbolKind::File, it->second);
  }
}

void QueryDatabase::ImportOrUpdate(
    std::vector<QueryType::DefUpdate>&& updates) {
  // This function runs on the querydb thread.

  for (auto& def : updates) {
    assert(!def.value.detailed_name.empty());

    auto it = usr_to_type.find(def.usr);
    assert(it != usr_to_type.end());

    assert(it->second.id >= 0 && it->second.id < types.size());
    QueryType& existing = types[it->second.id];
    if (!TryReplaceDef(existing.def, std::move(def.value))) {
      existing.def.push_front(std::move(def.value));
      UpdateSymbols(&existing.symbol_idx, SymbolKind::Type, it->second);
    }
  }
}

void QueryDatabase::ImportOrUpdate(
    std::vector<QueryFunc::DefUpdate>&& updates) {
  // This function runs on the querydb thread.

  for (auto& def : updates) {
    assert(!def.value.detailed_name.empty());

    auto it = usr_to_func.find(def.usr);
    assert(it != usr_to_func.end());

    assert(it->second.id >= 0 && it->second.id < funcs.size());
    QueryFunc& existing = funcs[it->second.id];
    if (!TryReplaceDef(existing.def, std::move(def.value))) {
      existing.def.push_front(std::move(def.value));
      UpdateSymbols(&existing.symbol_idx, SymbolKind::Func, it->second);
    }
  }
}

void QueryDatabase::ImportOrUpdate(std::vector<QueryVar::DefUpdate>&& updates) {
  // This function runs on the querydb thread.

  for (auto& def : updates) {
    assert(!def.value.detailed_name.empty());

    auto it = usr_to_var.find(def.usr);
    assert(it != usr_to_var.end());

    assert(it->second.id >= 0 && it->second.id < vars.size());
    QueryVar& existing = vars[it->second.id];
    if (!TryReplaceDef(existing.def, std::move(def.value))) {
      existing.def.push_front(std::move(def.value));
      if (!existing.def.front().is_local())
        UpdateSymbols(&existing.symbol_idx, SymbolKind::Var, it->second);
    }
  }
}

void QueryDatabase::UpdateSymbols(Maybe<Id<void>>* symbol_idx,
                                  SymbolKind kind,
                                  Id<void> idx) {
  if (!symbol_idx->HasValue()) {
    *symbol_idx = Id<void>(symbols.size());
    symbols.push_back(SymbolIdx{idx, kind});
  }
}

// For Func, the returned name does not include parameters.
std::string_view QueryDatabase::GetSymbolDetailedName(RawId symbol_idx) const {
  RawId idx = symbols[symbol_idx].id.id;
  switch (symbols[symbol_idx].kind) {
    default:
      break;
    case SymbolKind::File:
      if (files[idx].def)
        return files[idx].def->path;
      break;
    case SymbolKind::Func:
      if (const auto* def = funcs[idx].AnyDef())
        return def->DetailedName(false);
      break;
    case SymbolKind::Type:
      if (const auto* def = types[idx].AnyDef())
        return def->detailed_name;
      break;
    case SymbolKind::Var:
      if (const auto* def = vars[idx].AnyDef())
        return def->detailed_name;
      break;
  }
  return "";
}

std::string_view QueryDatabase::GetSymbolShortName(RawId symbol_idx) const {
  RawId idx = symbols[symbol_idx].id.id;
  switch (symbols[symbol_idx].kind) {
    default:
      break;
    case SymbolKind::File:
      if (files[idx].def)
        return files[idx].def->path;
      break;
    case SymbolKind::Func:
      if (const auto* def = funcs[idx].AnyDef())
        return def->ShortName();
      break;
    case SymbolKind::Type:
      if (const auto* def = types[idx].AnyDef())
        return def->ShortName();
      break;
    case SymbolKind::Var:
      if (const auto* def = vars[idx].AnyDef())
        return def->ShortName();
      break;
  }
  return "";
}

TEST_SUITE("query") {
  IndexUpdate GetDelta(IndexFile previous, IndexFile current) {
    QueryDatabase db;
    IdMap previous_map(&db, previous.id_cache);
    IdMap current_map(&db, current.id_cache);
    return IndexUpdate::CreateDelta(&previous_map, &current_map, &previous,
                                    &current);
  }

  TEST_CASE("remove defs") {
    IndexFile previous("foo.cc", "<empty>");
    IndexFile current("foo.cc", "<empty>");

    previous.Resolve(previous.ToTypeId(HashUsr("usr1")))->def.spell =
        Use(Range(Position(1, 0)), {}, {}, {}, {});
    previous.Resolve(previous.ToFuncId(HashUsr("usr2")))->def.spell =
        Use(Range(Position(2, 0)), {}, {}, {}, {});
    previous.Resolve(previous.ToVarId(HashUsr("usr3")))->def.spell =
        Use(Range(Position(3, 0)), {}, {}, {}, {});

    IndexUpdate update = GetDelta(previous, current);

    REQUIRE(update.types_removed == std::vector<Usr>{HashUsr("usr1")});
    REQUIRE(update.funcs_removed.size() == 1);
    REQUIRE(update.funcs_removed[0].usr == HashUsr("usr2"));
    REQUIRE(update.vars_removed.size() == 1);
    REQUIRE(update.vars_removed[0].usr == HashUsr("usr3"));
  }

  TEST_CASE("do not remove ref-only defs") {
    IndexFile previous("foo.cc", "<empty>");
    IndexFile current("foo.cc", "<empty>");

    previous.Resolve(previous.ToTypeId(HashUsr("usr1")))
        ->uses.push_back(Use{Range(Position(1, 0)), {}, {}, {}, {}});
    previous.Resolve(previous.ToFuncId(HashUsr("usr2")))
        ->uses.push_back(Use(Range(Position(2, 0)), {}, {}, {}, {}));
    previous.Resolve(previous.ToVarId(HashUsr("usr3")))
        ->uses.push_back(Use(Range(Position(3, 0)), {}, {}, {}, {}));

    IndexUpdate update = GetDelta(previous, current);

    REQUIRE(update.types_removed == std::vector<Usr>{});
    REQUIRE(update.funcs_removed.empty());
    REQUIRE(update.vars_removed.empty());
  }

  TEST_CASE("func callers") {
    IndexFile previous("foo.cc", "<empty>");
    IndexFile current("foo.cc", "<empty>");

    IndexFunc* pf = previous.Resolve(previous.ToFuncId(HashUsr("usr")));
    IndexFunc* cf = current.Resolve(current.ToFuncId(HashUsr("usr")));

    pf->uses.push_back(Use(Range(Position(1, 0)), {}, {}, {}, {}));
    cf->uses.push_back(Use(Range(Position(2, 0)), {}, {}, {}, {}));

    IndexUpdate update = GetDelta(previous, current);

    REQUIRE(update.funcs_removed.empty());
    REQUIRE(update.funcs_uses.size() == 1);
    REQUIRE(update.funcs_uses[0].id == QueryFuncId(0));
    REQUIRE(update.funcs_uses[0].to_remove.size() == 1);
    REQUIRE(update.funcs_uses[0].to_remove[0].range == Range(Position(1, 0)));
    REQUIRE(update.funcs_uses[0].to_add.size() == 1);
    REQUIRE(update.funcs_uses[0].to_add[0].range == Range(Position(2, 0)));
  }

  TEST_CASE("type usages") {
    IndexFile previous("foo.cc", "<empty>");
    IndexFile current("foo.cc", "<empty>");

    IndexType* pt = previous.Resolve(previous.ToTypeId(HashUsr("usr")));
    IndexType* ct = current.Resolve(current.ToTypeId(HashUsr("usr")));

    pt->uses.push_back(Use(Range(Position(1, 0)), {}, {}, {}, {}));
    ct->uses.push_back(Use(Range(Position(2, 0)), {}, {}, {}, {}));

    IndexUpdate update = GetDelta(previous, current);

    REQUIRE(update.types_removed == std::vector<Usr>{});
    REQUIRE(update.types_def_update.empty());
    REQUIRE(update.types_uses.size() == 1);
    REQUIRE(update.types_uses[0].to_remove.size() == 1);
    REQUIRE(update.types_uses[0].to_remove[0].range == Range(Position(1, 0)));
    REQUIRE(update.types_uses[0].to_add.size() == 1);
    REQUIRE(update.types_uses[0].to_add[0].range == Range(Position(2, 0)));
  }

  TEST_CASE("apply delta") {
    IndexFile previous("foo.cc", "<empty>");
    IndexFile current("foo.cc", "<empty>");

    IndexFunc* pf = previous.Resolve(previous.ToFuncId(HashUsr("usr")));
    IndexFunc* cf = current.Resolve(current.ToFuncId(HashUsr("usr")));
    pf->uses.push_back(Use(Range(Position(1, 0)), {}, {}, {}, {}));
    pf->uses.push_back(Use(Range(Position(2, 0)), {}, {}, {}, {}));
    cf->uses.push_back(Use(Range(Position(4, 0)), {}, {}, {}, {}));
    cf->uses.push_back(Use(Range(Position(5, 0)), {}, {}, {}, {}));

    QueryDatabase db;
    IdMap previous_map(&db, previous.id_cache);
    IdMap current_map(&db, current.id_cache);
    REQUIRE(db.funcs.size() == 1);

    IndexUpdate import_update =
        IndexUpdate::CreateDelta(nullptr, &previous_map, nullptr, &previous);
    IndexUpdate delta_update = IndexUpdate::CreateDelta(
        &previous_map, &current_map, &previous, &current);

    db.ApplyIndexUpdate(&import_update);
    REQUIRE(db.funcs[0].uses.size() == 2);
    REQUIRE(db.funcs[0].uses[0].range == Range(Position(1, 0)));
    REQUIRE(db.funcs[0].uses[1].range == Range(Position(2, 0)));

    db.ApplyIndexUpdate(&delta_update);
    REQUIRE(db.funcs[0].uses.size() == 2);
    REQUIRE(db.funcs[0].uses[0].range == Range(Position(4, 0)));
    REQUIRE(db.funcs[0].uses[1].range == Range(Position(5, 0)));
  }

  TEST_CASE("Remove variable with usage") {
    auto load_index_from_json = [](const char* json) {
      return Deserialize(SerializeFormat::Json, "foo.cc", json, "<empty>",
                         nullopt);
    };

    auto previous = load_index_from_json(R"RAW(
{
  "types": [
    {
      "id": 0,
      "usr": 17,
      "detailed_name": "",
      "short_name_offset": 0,
      "short_name_size": 0,
      "kind": 0,
      "hover": "",
      "comments": "",
      "parents": [],
      "derived": [],
      "types": [],
      "funcs": [],
      "vars": [],
      "instances": [
        0
      ],
      "uses": []
    }
  ],
  "funcs": [
    {
      "id": 0,
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "short_name_offset": 5,
      "short_name_size": 3,
      "kind": 12,
      "storage": 1,
      "hover": "",
      "comments": "",
      "declarations": [],
      "spell": "1:6-1:9|-1|1|2",
      "extent": "1:1-4:2|-1|1|0",
      "base": [],
      "derived": [],
      "locals": [],
      "uses": [],
      "callees": []
    }
  ],
  "vars": [
    {
      "id": 0,
      "usr": 16837348799350457167,
      "detailed_name": "int a",
      "short_name_offset": 4,
      "short_name_size": 1,
      "hover": "",
      "comments": "",
      "declarations": [],
      "spell": "2:7-2:8|0|3|2",
      "extent": "2:3-2:8|0|3|2",
      "type": 0,
      "uses": [
        "3:3-3:4|0|3|4"
      ],
      "kind": 13,
      "storage": 1
    }
  ]
}
    )RAW");

    auto current = load_index_from_json(R"RAW(
{
  "types": [],
  "funcs": [
    {
      "id": 0,
      "usr": 4259594751088586730,
      "detailed_name": "void foo()",
      "short_name_offset": 5,
      "short_name_size": 3,
      "kind": 12,
      "storage": 1,
      "hover": "",
      "comments": "",
      "declarations": [],
      "spell": "1:6-1:9|-1|1|2",
      "extent": "1:1-5:2|-1|1|0",
      "base": [],
      "derived": [],
      "locals": [],
      "uses": [],
      "callees": []
    }
  ],
  "vars": []
}
    )RAW");

    // Validate previous/current were parsed.
    REQUIRE(previous->vars.size() == 1);
    REQUIRE(current->vars.size() == 0);

    QueryDatabase db;

    // Apply initial file.
    {
      IdMap previous_map(&db, previous->id_cache);
      IndexUpdate import_update = IndexUpdate::CreateDelta(
          nullptr, &previous_map, nullptr, previous.get());
      db.ApplyIndexUpdate(&import_update);
    }

    REQUIRE(db.vars.size() == 1);
    REQUIRE(db.vars[0].uses.size() == 1);

    // Apply change.
    {
      IdMap previous_map(&db, previous->id_cache);
      IdMap current_map(&db, current->id_cache);
      IndexUpdate delta_update = IndexUpdate::CreateDelta(
          &previous_map, &current_map, previous.get(), current.get());
      db.ApplyIndexUpdate(&delta_update);
    }
    REQUIRE(db.vars.size() == 1);
    REQUIRE(db.vars[0].uses.size() == 0);
  }
}
