#include "query.h"

#include "indexer.h"
#include "serializer.h"
#include "serializers/json.h"

#include <doctest/doctest.h>
#include <optional>
#include <loguru.hpp>

#include <cassert>
#include <cstdint>
#include <functional>
#include <iterator>
#include <string>
#include <unordered_map>
#include <unordered_set>

// TODO: Make all copy constructors explicit.

// Used by |HANDLE_MERGEABLE| so only |range| is needed.
MAKE_HASHABLE(Range, t.start, t.end);
MAKE_HASHABLE(Use, t.range);

template <typename TVisitor, typename TValue>
void Reflect(TVisitor& visitor, MergeableUpdate<TValue>& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(usr);
  REFLECT_MEMBER(to_remove);
  REFLECT_MEMBER(to_add);
  REFLECT_MEMBER_END();
}

template <typename TVisitor, typename T>
void Reflect(TVisitor& visitor, WithUsr<T>& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(usr);
  REFLECT_MEMBER(value);
  REFLECT_MEMBER_END();
}

template <typename TVisitor, typename T>
void Reflect(TVisitor& visitor, WithFileContent<T>& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(value);
  REFLECT_MEMBER(file_content);
  REFLECT_MEMBER_END();
}

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

namespace {

void AssignFileId(int file_id, SymbolRef& ref) {
  if (ref.kind == SymbolKind::File)
    ref.usr = file_id;
}

void AssignFileId(int file_id, Use& use) {
  if (use.kind == SymbolKind::File)
    use.usr = file_id;
  use.file_id = file_id;
}

template <typename T>
void AssignFileId(int file_id, T&) {}

template <typename T>
void AssignFileId(int file_id, Maybe<T>& x) {
  if (x)
    AssignFileId(file_id, *x);
}

template <typename T>
void AssignFileId(int file_id, std::vector<T>& xs) {
  for (T& x : xs)
    AssignFileId(file_id, x);
}


void AddRange(int file_id, std::vector<Use>& into, const std::vector<Use>& from) {
  into.reserve(into.size() + from.size());
  for (Use use : from) {
    use.file_id = file_id;
    into.push_back(use);
  }
}

void AddRange(int _, std::vector<Usr>& into, const std::vector<Usr>& from) {
  into.insert(into.end(), from.begin(), from.end());
}

template <typename T>
void RemoveRange(std::vector<T>& dest, const std::vector<T>& to_remove) {
  if (to_remove.size()) {
    std::unordered_set<T> to_remove_set(to_remove.begin(), to_remove.end());
    dest.erase(
      std::remove_if(dest.begin(), dest.end(),
        [&](const T& t) { return to_remove_set.count(t) > 0; }),
      dest.end());
  }
}

// Compares |previous| and |current|, adding all elements that are in |previous|
// but not |current| to |removed|, and all elements that are in |current| but
// not |previous| to |added|.
//
// Returns true iff |removed| or |added| are non-empty.
template <typename T>
bool ComputeDifferenceForUpdate(std::vector<T> previous,
                                std::vector<T> current,
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
void CompareGroups(std::unordered_map<Usr, T>& prev,
                   std::unordered_map<Usr, T>& curr,
                   std::function<void(T&)> on_remove,
                   std::function<void(T&)> on_add,
                   std::function<void(T&, T&)> on_found) {
  for (auto& it : prev) {
    auto it1 = curr.find(it.first);
    if (it1 != curr.end())
      on_found(it.second, it1->second);
    else
      on_remove(it.second);
  }
  for (auto& it : curr)
    if (!prev.count(it.first))
      on_add(it.second);
}

QueryFile::DefUpdate BuildFileDefUpdate(const IndexFile& indexed) {
  QueryFile::Def def;
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

  auto add_all_symbols = [&](Use use, Usr usr, SymbolKind kind) {
    def.all_symbols.push_back(SymbolRef(use.range, usr, kind, use.role));
  };
  auto add_outline = [&](Use use, Usr usr, SymbolKind kind) {
    def.outline.push_back(SymbolRef(use.range, usr, kind, use.role));
  };

  for (auto& it : indexed.usr2type) {
    const IndexType& type = it.second;
    if (type.def.spell)
      add_all_symbols(*type.def.spell, type.usr, SymbolKind::Type);
    if (type.def.extent)
      add_outline(*type.def.extent, type.usr, SymbolKind::Type);
    for (Use decl : type.declarations) {
      add_all_symbols(decl, type.usr, SymbolKind::Type);
      // Constructor positions have references to the class,
      // which we do not want to show in textDocument/documentSymbol
      if (!(decl.role & Role::Reference))
        add_outline(decl, type.usr, SymbolKind::Type);
    }
    for (Use use : type.uses)
      add_all_symbols(use, type.usr, SymbolKind::Type);
  }
  for (auto& it: indexed.usr2func) {
    const IndexFunc& func = it.second;
    if (func.def.spell)
      add_all_symbols(*func.def.spell, func.usr, SymbolKind::Func);
    if (func.def.extent)
      add_outline(*func.def.extent, func.usr, SymbolKind::Func);
    for (Use use : func.declarations) {
      add_all_symbols(use, func.usr, SymbolKind::Func);
      add_outline(use, func.usr, SymbolKind::Func);
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
      add_all_symbols(use, func.usr, SymbolKind::Func);
    }
  }
  for (auto& it : indexed.usr2var) {
    const IndexVar& var = it.second;
    if (var.def.spell)
      add_all_symbols(*var.def.spell, var.usr, SymbolKind::Var);
    if (var.def.extent)
      add_outline(*var.def.extent, var.usr, SymbolKind::Var);
    for (Use decl : var.declarations) {
      add_all_symbols(decl, var.usr, SymbolKind::Var);
      add_outline(decl, var.usr, SymbolKind::Var);
    }
    for (Use use : var.uses)
      add_all_symbols(use, var.usr, SymbolKind::Var);
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

// Returns true if an element with the same file is found.
template <typename Q>
bool TryReplaceDef(std::forward_list<Q>& def_list, Q&& def) {
  for (auto& def1 : def_list)
    if (def1.file_id == def.file_id) {
      if (!def1.spell || def.spell) {
        def1 = std::move(def);
      }
      return true;
    }
  return false;
}

}  // namespace

// ----------------------
// INDEX THREAD FUNCTIONS
// ----------------------

// static
IndexUpdate IndexUpdate::CreateDelta(IndexFile* previous,
                                     IndexFile* current) {
  IndexUpdate r;
  static IndexFile empty(current->path, "<empty>");
  if (!previous)
    previous = &empty;

// |query_name| is the name of the variable on the query type.
// |index_name| is the name of the variable on the index type.
// |type| is the type of the variable.
#define PROCESS_DIFF(type_id, query_name, index_name, type)  \
  {                                                          \
    /* Check for changes. */                                 \
    std::vector<type> removed, added;                        \
    bool did_add = ComputeDifferenceForUpdate(               \
        prev.index_name, curr.index_name, &removed, &added); \
    if (did_add) {                                           \
      r.query_name.push_back(MergeableUpdate<type>(          \
          curr.usr, std::move(removed), std::move(added)));  \
    }                                                        \
  }
  // File
  r.files_def_update = BuildFileDefUpdate(*current);

  // **NOTE** We only remove entries if they were defined in the previous index.
  // For example, if a type is included from another file it will be defined
  // simply so we can attribute the usage/reference to it. If the reference goes
  // away we don't want to remove the type/func/var usage.

  // Functions
  CompareGroups<IndexFunc>(
      previous->usr2func, current->usr2func,
      /*onRemoved:*/
      [&r](IndexFunc& func) {
        if (func.def.spell)
          r.funcs_removed.push_back(func.usr);
        if (func.declarations.size())
          r.funcs_declarations.push_back(UseUpdate{func.usr, func.declarations, {}});
        if (func.uses.size())
          r.funcs_uses.push_back(UseUpdate{func.usr, func.uses, {}});
        if (func.derived.size())
          r.funcs_derived.push_back(UsrUpdate{func.usr, func.derived, {}});
      },
      /*onAdded:*/
      [&r](IndexFunc& func) {
        if (func.def.detailed_name.size())
          r.funcs_def_update.emplace_back(func.usr, func.def);
        if (func.declarations.size())
          r.funcs_declarations.push_back(UseUpdate{func.usr, {}, func.declarations});
        if (func.uses.size())
          r.funcs_uses.push_back(UseUpdate{func.usr, {}, func.uses});
        if (func.derived.size())
          r.funcs_derived.push_back(UsrUpdate{func.usr, {}, func.derived});
      },
      /*onFound:*/
      [&r](IndexFunc& prev, IndexFunc& curr) {
        if (curr.def.detailed_name.size() && !(prev.def == curr.def))
          r.funcs_def_update.emplace_back(curr.usr, curr.def);

        PROCESS_DIFF(QueryFuncId, funcs_declarations, declarations, Use);
        PROCESS_DIFF(QueryFuncId, funcs_uses, uses, Use);
        PROCESS_DIFF(QueryFuncId, funcs_derived, derived, Usr);
      });

  // Types
  CompareGroups<IndexType>(
      previous->usr2type, current->usr2type,
      /*onRemoved:*/
      [&r](IndexType& type) {
        if (type.def.spell)
          r.types_removed.push_back(type.usr);
        if (type.declarations.size())
          r.types_declarations.push_back(UseUpdate{type.usr, type.declarations, {}});
        if (type.uses.size())
          r.types_uses.push_back(UseUpdate{type.usr, type.uses, {}});
        if (type.derived.size())
          r.types_derived.push_back(UsrUpdate{type.usr, type.derived, {}});
        if (type.instances.size())
          r.types_instances.push_back(UsrUpdate{type.usr, type.instances, {}});
      },
      /*onAdded:*/
      [&r](IndexType& type) {
        if (type.def.detailed_name.size())
          r.types_def_update.emplace_back(type.usr, type.def);
        if (type.declarations.size())
          r.types_declarations.push_back(UseUpdate{type.usr, {}, type.declarations});
        if (type.uses.size())
          r.types_uses.push_back(UseUpdate{type.usr, {}, type.uses});
        if (type.derived.size())
          r.types_derived.push_back(UsrUpdate{type.usr, {}, type.derived});
        if (type.instances.size())
          r.types_instances.push_back(UsrUpdate{type.usr, {}, type.instances});
      },
      /*onFound:*/
      [&r](IndexType& prev, IndexType& curr) {
        if (curr.def.detailed_name.size() && !(prev.def == curr.def))
          r.types_def_update.emplace_back(curr.usr, curr.def);

        PROCESS_DIFF(QueryTypeId, types_declarations, declarations, Use);
        PROCESS_DIFF(QueryTypeId, types_uses, uses, Use);
        PROCESS_DIFF(QueryTypeId, types_derived, derived, Usr);
        PROCESS_DIFF(QueryTypeId, types_instances, instances, Usr);
      });

  // Variables
  CompareGroups<IndexVar>(
      previous->usr2var, current->usr2var,
      /*onRemoved:*/
      [&r](IndexVar& var) {
        if (var.def.spell)
          r.vars_removed.push_back(var.usr);
        if (!var.declarations.empty())
          r.vars_declarations.push_back(UseUpdate{var.usr, var.declarations, {}});
        if (!var.uses.empty())
          r.vars_uses.push_back(UseUpdate{var.usr, var.uses, {}});
      },
      /*onAdded:*/
      [&r](IndexVar& var) {
        if (var.def.detailed_name.size())
          r.vars_def_update.emplace_back(var.usr, var.def);
        if (var.declarations.size())
          r.vars_declarations.push_back(UseUpdate{var.usr, {}, var.declarations});
        if (var.uses.size())
          r.vars_uses.push_back(UseUpdate{var.usr, {}, var.uses});
      },
      /*onFound:*/
      [&r](IndexVar& prev, IndexVar& curr) {
        if (curr.def.detailed_name.size() && !(prev.def == curr.def))
          r.vars_def_update.emplace_back(curr.usr, curr.def);

        PROCESS_DIFF(QueryVarId, vars_declarations, declarations, Use);
        PROCESS_DIFF(QueryVarId, vars_uses, uses, Use);
      });

  return r;
#undef PROCESS_DIFF
}

std::string IndexUpdate::ToString() {
  rapidjson::StringBuffer output;
  rapidjson::Writer<rapidjson::StringBuffer> writer(output);
  JsonWriter json_writer(&writer);
  IndexUpdate& update = *this;
  Reflect(json_writer, update);
  return output.GetString();
}

// ------------------------
// QUERYDB THREAD FUNCTIONS
// ------------------------

void QueryDatabase::RemoveUsrs(SymbolKind usr_kind,
                               const std::vector<Usr>& to_remove) {
  switch (usr_kind) {
    case SymbolKind::Type: {
      for (Usr usr : to_remove) {
        QueryType& type = usr2type[usr];
        if (type.symbol_idx >= 0)
          symbols[type.symbol_idx].kind = SymbolKind::Invalid;
        type.def.clear();
      }
      break;
    }
    default:
      break;
  }
}

void QueryDatabase::RemoveUsrs(
    SymbolKind kind,
    int file_id,
    const std::vector<Usr>& to_remove) {
  switch (kind) {
    case SymbolKind::Func: {
      for (auto usr : to_remove) {
        QueryFunc& func = Func(usr);
        func.def.remove_if([=](const QueryFunc::Def& def) {
          return def.file_id == file_id;
        });
      }
      break;
    }
    case SymbolKind::Var: {
      for (auto usr : to_remove) {
        QueryVar& var = Var(usr);
        var.def.remove_if([=](const QueryVar::Def& def) {
          return def.file_id == file_id;
        });
      }
      break;
    }
    default:
      assert(false);
      break;
  }
}

void QueryDatabase::ApplyIndexUpdate(IndexUpdate* u) {
// This function runs on the querydb thread.

// Example types:
//  storage_name       =>  std::vector<std::optional<QueryType>>
//  merge_update       =>  QueryType::DerivedUpdate =>
//  MergeableUpdate<QueryTypeId, QueryTypeId> def                =>  QueryType
//  def->def_var_name  =>  std::vector<QueryTypeId>
#define HANDLE_MERGEABLE(update_var_name, def_var_name, storage_name) \
  for (auto merge_update : u->update_var_name) {                      \
    auto& entity = storage_name[merge_update.usr];                    \
    AssignFileId(u->file_id, merge_update.to_add);                    \
    AddRange(u->file_id, entity.def_var_name, merge_update.to_add);   \
    RemoveRange(entity.def_var_name, merge_update.to_remove);         \
  }

  if (u->files_removed)
    files[name2file_id[LowerPathIfInsensitive(*u->files_removed)]].def =
        std::nullopt;
  u->file_id = u->files_def_update ? Update(std::move(*u->files_def_update)) : -1;

  RemoveUsrs(SymbolKind::Func, u->file_id, u->funcs_removed);
  Update(u->file_id, std::move(u->funcs_def_update));
  HANDLE_MERGEABLE(funcs_declarations, declarations, usr2func);
  HANDLE_MERGEABLE(funcs_derived, derived, usr2func);
  HANDLE_MERGEABLE(funcs_uses, uses, usr2func);

  RemoveUsrs(SymbolKind::Type, u->types_removed);
  Update(u->file_id, std::move(u->types_def_update));
  HANDLE_MERGEABLE(types_declarations, declarations, usr2type);
  HANDLE_MERGEABLE(types_derived, derived, usr2type);
  HANDLE_MERGEABLE(types_instances, instances, usr2type);
  HANDLE_MERGEABLE(types_uses, uses, usr2type);

  RemoveUsrs(SymbolKind::Var, u->file_id, u->vars_removed);
  Update(u->file_id, std::move(u->vars_def_update));
  HANDLE_MERGEABLE(vars_declarations, declarations, usr2var);
  HANDLE_MERGEABLE(vars_uses, uses, usr2var);

#undef HANDLE_MERGEABLE
}

int QueryDatabase::Update(QueryFile::DefUpdate&& u) {
  int id = files.size();
  auto it = name2file_id.try_emplace(LowerPathIfInsensitive(u.value.path), id);
  if (it.second)
    files.emplace_back().id = id;
  QueryFile& existing = files[it.first->second];
  existing.def = u.value;
  UpdateSymbols(&existing.symbol_idx, SymbolKind::File, it.first->second);
  return existing.id;
}

void QueryDatabase::Update(int file_id, std::vector<QueryFunc::DefUpdate>&& updates) {
  for (auto& u : updates) {
    auto& def = u.value;
    assert(!def.detailed_name.empty());
    AssignFileId(file_id, def.spell);
    AssignFileId(file_id, def.extent);
    AssignFileId(file_id, def.callees);
    QueryFunc& existing = Func(u.usr);
    if (!TryReplaceDef(existing.def, std::move(def))) {
      existing.def.push_front(std::move(def));
      UpdateSymbols(&existing.symbol_idx, SymbolKind::Type, existing.usr);
    }
  }
}

void QueryDatabase::Update(int file_id, std::vector<QueryType::DefUpdate>&& updates) {
  for (auto& u : updates) {
    auto& def = u.value;
    assert(!def.detailed_name.empty());
    AssignFileId(file_id, def.spell);
    AssignFileId(file_id, def.extent);
    QueryType& existing = Type(u.usr);
    if (!TryReplaceDef(existing.def, std::move(def))) {
      existing.def.push_front(std::move(def));
      UpdateSymbols(&existing.symbol_idx, SymbolKind::Type, existing.usr);
    }
  }
}

void QueryDatabase::Update(int file_id, std::vector<QueryVar::DefUpdate>&& updates) {
  for (auto& u : updates) {
    auto& def = u.value;
    assert(!def.detailed_name.empty());
    AssignFileId(file_id, def.spell);
    AssignFileId(file_id, def.extent);
    QueryVar& existing = Var(u.usr);
    if (!TryReplaceDef(existing.def, std::move(def))) {
      existing.def.push_front(std::move(def));
      if (!existing.def.front().is_local())
        UpdateSymbols(&existing.symbol_idx, SymbolKind::Var, existing.usr);
    }
  }
}

void QueryDatabase::UpdateSymbols(int* symbol_idx,
                                  SymbolKind kind,
                                  Usr usr) {
  if (*symbol_idx < 0) {
    *symbol_idx = symbols.size();
    symbols.push_back(SymbolIdx{usr, kind});
  }
}

std::string_view QueryDatabase::GetSymbolName(int symbol_idx,
                                              bool qualified) {
  Usr usr = symbols[symbol_idx].usr;
  switch (symbols[symbol_idx].kind) {
    default:
      break;
    case SymbolKind::File:
      if (files[usr].def)
        return files[usr].def->path;
      break;
    case SymbolKind::Func:
      if (const auto* def = Func(usr).AnyDef())
        return def->Name(qualified);
      break;
    case SymbolKind::Type:
      if (const auto* def = Type(usr).AnyDef())
        return def->Name(qualified);
      break;
    case SymbolKind::Var:
      if (const auto* def = Var(usr).AnyDef())
        return def->Name(qualified);
      break;
  }
  return "";
}
