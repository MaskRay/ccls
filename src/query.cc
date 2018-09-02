// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "query.h"

#include "indexer.h"
#include "serializer.h"
#include "serializers/json.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace {

void AssignFileId(const Lid2file_id &lid2file_id, int file_id, SymbolRef &ref) {
  if (ref.kind == SymbolKind::File)
    ref.usr = file_id;
}

void AssignFileId(const Lid2file_id &lid2file_id, int file_id, Use &use) {
  if (use.kind == SymbolKind::File)
    use.usr = file_id;
  if (use.file_id == -1)
    use.file_id = file_id;
  else
    use.file_id = lid2file_id.find(use.file_id)->second;
}

template <typename T>
void AssignFileId(const Lid2file_id &, int file_id, T &) {}

template <typename T>
void AssignFileId(const Lid2file_id &lid2file_id, int file_id, Maybe<T> &x) {
  if (x)
    AssignFileId(lid2file_id, file_id, *x);
}

template <typename T>
void AssignFileId(const Lid2file_id &lid2file_id, int file_id,
                  std::vector<T> &xs) {
  for (T &x : xs)
    AssignFileId(lid2file_id, file_id, x);
}

void AddRange(std::vector<Use> &into, const std::vector<Use> &from) {
  into.reserve(into.size() + from.size());
  for (Use use : from)
    into.push_back(use);
}

void AddRange(std::vector<Usr> &into, const std::vector<Usr> &from) {
  into.insert(into.end(), from.begin(), from.end());
}

template <typename T>
void RemoveRange(std::vector<T> &from, const std::vector<T> &to_remove) {
  if (to_remove.size()) {
    std::unordered_set<T> to_remove_set(to_remove.begin(), to_remove.end());
    from.erase(
        std::remove_if(from.begin(), from.end(),
                       [&](const T &t) { return to_remove_set.count(t) > 0; }),
        from.end());
  }
}

QueryFile::DefUpdate BuildFileDefUpdate(const IndexFile &indexed) {
  QueryFile::Def def;
  def.path = std::move(indexed.path);
  def.args = std::move(indexed.args);
  def.includes = std::move(indexed.includes);
  def.skipped_ranges = std::move(indexed.skipped_ranges);
  def.dependencies.reserve(indexed.dependencies.size());
  for (auto &dep : indexed.dependencies)
    def.dependencies.push_back(dep.first());
  def.language = indexed.language;

  auto add_all_symbols = [&](Use use, Usr usr, SymbolKind kind) {
    def.all_symbols.push_back(SymbolRef{{use.range, usr, kind, use.role}});
  };
  auto add_outline = [&](Use use, Usr usr, SymbolKind kind) {
    def.outline.push_back(SymbolRef{{use.range, usr, kind, use.role}});
  };

  for (auto &it : indexed.usr2type) {
    const IndexType &type = it.second;
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
      if (use.file_id == -1)
        add_all_symbols(use, type.usr, SymbolKind::Type);
  }
  for (auto &it : indexed.usr2func) {
    const IndexFunc &func = it.second;
    if (func.def.spell)
      add_all_symbols(*func.def.spell, func.usr, SymbolKind::Func);
    if (func.def.extent)
      add_outline(*func.def.extent, func.usr, SymbolKind::Func);
    for (Use use : func.declarations) {
      add_all_symbols(use, func.usr, SymbolKind::Func);
      add_outline(use, func.usr, SymbolKind::Func);
    }
    for (Use use : func.uses)
      if (use.file_id == -1) {
        // Make ranges of implicit function calls larger (spanning one more
        // column to the left/right). This is hacky but useful. e.g.
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
  for (auto &it : indexed.usr2var) {
    const IndexVar &var = it.second;
    if (var.def.spell)
      add_all_symbols(*var.def.spell, var.usr, SymbolKind::Var);
    if (var.def.extent)
      add_outline(*var.def.extent, var.usr, SymbolKind::Var);
    for (Use decl : var.declarations) {
      add_all_symbols(decl, var.usr, SymbolKind::Var);
      add_outline(decl, var.usr, SymbolKind::Var);
    }
    for (Use use : var.uses)
      if (use.file_id == -1)
        add_all_symbols(use, var.usr, SymbolKind::Var);
  }

  std::sort(def.outline.begin(), def.outline.end(),
            [](const SymbolRef &a, const SymbolRef &b) {
              return a.range.start < b.range.start;
            });
  std::sort(def.all_symbols.begin(), def.all_symbols.end(),
            [](const SymbolRef &a, const SymbolRef &b) {
              return a.range.start < b.range.start;
            });

  return {std::move(def), std::move(indexed.file_contents)};
}

// Returns true if an element with the same file is found.
template <typename Q>
bool TryReplaceDef(llvm::SmallVectorImpl<Q> &def_list, Q &&def) {
  for (auto &def1 : def_list)
    if (def1.file_id == def.file_id) {
      def1 = std::move(def);
      return true;
    }
  return false;
}

} // namespace

IndexUpdate IndexUpdate::CreateDelta(IndexFile *previous, IndexFile *current) {
  IndexUpdate r;
  static IndexFile empty(llvm::sys::fs::UniqueID(0, 0), current->path,
                         "<empty>");
  if (previous)
    r.prev_lid2path = std::move(previous->lid2path);
  else
    previous = &empty;
  r.lid2path = std::move(current->lid2path);
  r.files_def_update = BuildFileDefUpdate(std::move(*current));

  r.funcs_hint = int(current->usr2func.size() - previous->usr2func.size());
  for (auto &it : previous->usr2func) {
    auto &func = it.second;
    if (func.def.detailed_name[0])
      r.funcs_removed.push_back(func.usr);
    r.funcs_declarations[func.usr].first = std::move(func.declarations);
    r.funcs_uses[func.usr].first = std::move(func.uses);
    r.funcs_derived[func.usr].first = std::move(func.derived);
  }
  for (auto &it : current->usr2func) {
    auto &func = it.second;
    if (func.def.detailed_name[0])
      r.funcs_def_update.emplace_back(it.first, func.def);
    r.funcs_declarations[func.usr].second = std::move(func.declarations);
    r.funcs_uses[func.usr].second = std::move(func.uses);
    r.funcs_derived[func.usr].second = std::move(func.derived);
  }

  r.types_hint = int(current->usr2type.size() - previous->usr2type.size());
  for (auto &it : previous->usr2type) {
    auto &type = it.second;
    if (type.def.detailed_name[0])
      r.types_removed.push_back(type.usr);
    r.types_declarations[type.usr].first = std::move(type.declarations);
    r.types_uses[type.usr].first = std::move(type.uses);
    r.types_derived[type.usr].first = std::move(type.derived);
    r.types_instances[type.usr].first = std::move(type.instances);
  };
  for (auto &it : current->usr2type) {
    auto &type = it.second;
    if (type.def.detailed_name[0])
      r.types_def_update.emplace_back(it.first, type.def);
    r.types_declarations[type.usr].second = std::move(type.declarations);
    r.types_uses[type.usr].second = std::move(type.uses);
    r.types_derived[type.usr].second = std::move(type.derived);
    r.types_instances[type.usr].second = std::move(type.instances);
  };

  r.vars_hint = int(current->usr2var.size() - previous->usr2var.size());
  for (auto &it : previous->usr2var) {
    auto &var = it.second;
    if (var.def.detailed_name[0])
      r.vars_removed.push_back(var.usr);
    r.vars_declarations[var.usr].first = std::move(var.declarations);
    r.vars_uses[var.usr].first = std::move(var.uses);
  }
  for (auto &it : current->usr2var) {
    auto &var = it.second;
    if (var.def.detailed_name[0])
      r.vars_def_update.emplace_back(it.first, var.def);
    r.vars_declarations[var.usr].second = std::move(var.declarations);
    r.vars_uses[var.usr].second = std::move(var.uses);
  }

  return r;
}

void DB::RemoveUsrs(SymbolKind kind, int file_id,
                    const std::vector<Usr> &to_remove) {
  switch (kind) {
  case SymbolKind::Func: {
    for (Usr usr : to_remove) {
      // FIXME
      if (!HasFunc(usr))
        continue;
      QueryFunc &func = Func(usr);
      auto it = llvm::find_if(func.def, [=](const QueryFunc::Def &def) {
        return def.file_id == file_id;
      });
      if (it != func.def.end())
        func.def.erase(it);
    }
    break;
  }
  case SymbolKind::Type: {
    for (Usr usr : to_remove) {
      // FIXME
      if (!HasType(usr))
        continue;
      QueryType &type = Type(usr);
      auto it = llvm::find_if(type.def, [=](const QueryType::Def &def) {
        return def.file_id == file_id;
      });
      if (it != type.def.end())
        type.def.erase(it);
    }
    break;
  }
  case SymbolKind::Var: {
    for (Usr usr : to_remove) {
      // FIXME
      if (!HasVar(usr))
        continue;
      QueryVar &var = Var(usr);
      auto it = llvm::find_if(var.def, [=](const QueryVar::Def &def) {
        return def.file_id == file_id;
      });
      if (it != var.def.end())
        var.def.erase(it);
    }
    break;
  }
  default:
    break;
  }
}

void DB::ApplyIndexUpdate(IndexUpdate *u) {
#define REMOVE_ADD(C, F)                                                       \
  for (auto &it : u->C##s_##F) {                                               \
    auto R = C##_usr.try_emplace({it.first}, C##_usr.size());                  \
    if (R.second)                                                              \
      C##s.emplace_back().usr = it.first;                                      \
    auto &entity = C##s[R.first->second];                                      \
    AssignFileId(prev_lid2file_id, u->file_id, it.second.first);               \
    RemoveRange(entity.F, it.second.first);                                    \
    AssignFileId(lid2file_id, u->file_id, it.second.second);                   \
    AddRange(entity.F, it.second.second);                                      \
  }

  std::unordered_map<int, int> prev_lid2file_id, lid2file_id;
  for (auto &[lid, path] : u->prev_lid2path)
    prev_lid2file_id[lid] = GetFileId(path);
  for (auto &[lid, path] : u->lid2path)
    lid2file_id[lid] = GetFileId(path);

  auto UpdateUses = [&](Usr usr, SymbolKind kind,
                        llvm::DenseMap<WrappedUsr, int> &entity_usr,
                        auto &entities, auto &p) {
    auto R = entity_usr.try_emplace({usr}, entity_usr.size());
    if (R.second)
      vars.emplace_back().usr = usr;
    auto &entity = entities[R.first->second];
    for (Use &use : p.first) {
      if (use.file_id == -1)
        use.file_id = u->file_id;
      else {
        use.file_id = prev_lid2file_id.find(use.file_id)->second;
        files[use.file_id]
            .symbol2refcnt[SymbolRef{{use.range, usr, kind, use.role}}]--;
      }
    }
    RemoveRange(entity.uses, p.first);
    for (Use &use : p.second) {
      if (use.file_id == -1)
        use.file_id = u->file_id;
      else {
        use.file_id = lid2file_id.find(use.file_id)->second;
        files[use.file_id]
            .symbol2refcnt[SymbolRef{{use.range, usr, kind, use.role}}]++;
      }
    }
    AddRange(entity.uses, p.second);
  };

  if (u->files_removed)
    files[name2file_id[LowerPathIfInsensitive(*u->files_removed)]].def =
        std::nullopt;
  u->file_id =
      u->files_def_update ? Update(std::move(*u->files_def_update)) : -1;

  const double grow = 1.3;
  size_t t;

  if ((t = funcs.size() + u->funcs_hint) > funcs.capacity()) {
    t = size_t(t * grow);
    funcs.reserve(t);
    func_usr.reserve(t);
  }
  RemoveUsrs(SymbolKind::Func, u->file_id, u->funcs_removed);
  Update(lid2file_id, u->file_id, std::move(u->funcs_def_update));
  REMOVE_ADD(func, declarations);
  REMOVE_ADD(func, derived);
  for (auto &[usr, p] : u->funcs_uses)
    UpdateUses(usr, SymbolKind::Func, func_usr, funcs, p);

  if ((t = types.size() + u->types_hint) > types.capacity()) {
    t = size_t(t * grow);
    types.reserve(t);
    type_usr.reserve(t);
  }
  RemoveUsrs(SymbolKind::Type, u->file_id, u->types_removed);
  Update(lid2file_id, u->file_id, std::move(u->types_def_update));
  REMOVE_ADD(type, declarations);
  REMOVE_ADD(type, derived);
  REMOVE_ADD(type, instances);
  for (auto &[usr, p] : u->types_uses)
    UpdateUses(usr, SymbolKind::Type, type_usr, types, p);

  if ((t = vars.size() + u->vars_hint) > vars.capacity()) {
    t = size_t(t * grow);
    vars.reserve(t);
    var_usr.reserve(t);
  }
  RemoveUsrs(SymbolKind::Var, u->file_id, u->vars_removed);
  Update(lid2file_id, u->file_id, std::move(u->vars_def_update));
  REMOVE_ADD(var, declarations);
  for (auto &[usr, p] : u->vars_uses)
    UpdateUses(usr, SymbolKind::Var, var_usr, vars, p);

#undef REMOVE_ADD
}

int DB::GetFileId(const std::string &path) {
  auto it = name2file_id.try_emplace(LowerPathIfInsensitive(path));
  if (it.second) {
    int id = files.size();
    it.first->second = files.emplace_back().id = id;
  }
  return it.first->second;
}

int DB::Update(QueryFile::DefUpdate &&u) {
  int file_id = GetFileId(u.first.path);
  files[file_id].def = u.first;
  return file_id;
}

void DB::Update(const Lid2file_id &lid2file_id, int file_id,
                std::vector<std::pair<Usr, QueryFunc::Def>> &&us) {
  for (auto &u : us) {
    auto &def = u.second;
    assert(def.detailed_name[0]);
    u.second.file_id = file_id;
    AssignFileId(lid2file_id, file_id, def.spell);
    AssignFileId(lid2file_id, file_id, def.extent);
    AssignFileId(lid2file_id, file_id, def.callees);
    auto R = func_usr.try_emplace({u.first}, func_usr.size());
    if (R.second)
      funcs.emplace_back();
    QueryFunc &existing = funcs[R.first->second];
    existing.usr = u.first;
    if (!TryReplaceDef(existing.def, std::move(def)))
      existing.def.push_back(std::move(def));
  }
}

void DB::Update(const Lid2file_id &lid2file_id, int file_id,
                std::vector<std::pair<Usr, QueryType::Def>> &&us) {
  for (auto &u : us) {
    auto &def = u.second;
    assert(def.detailed_name[0]);
    u.second.file_id = file_id;
    AssignFileId(lid2file_id, file_id, def.spell);
    AssignFileId(lid2file_id, file_id, def.extent);
    auto R = type_usr.try_emplace({u.first}, type_usr.size());
    if (R.second)
      types.emplace_back();
    QueryType &existing = types[R.first->second];
    existing.usr = u.first;
    if (!TryReplaceDef(existing.def, std::move(def)))
      existing.def.push_back(std::move(def));
  }
}

void DB::Update(const Lid2file_id &lid2file_id, int file_id,
                std::vector<std::pair<Usr, QueryVar::Def>> &&us) {
  for (auto &u : us) {
    auto &def = u.second;
    assert(def.detailed_name[0]);
    u.second.file_id = file_id;
    AssignFileId(lid2file_id, file_id, def.spell);
    AssignFileId(lid2file_id, file_id, def.extent);
    auto R = var_usr.try_emplace({u.first}, var_usr.size());
    if (R.second)
      vars.emplace_back();
    QueryVar &existing = vars[R.first->second];
    existing.usr = u.first;
    if (!TryReplaceDef(existing.def, std::move(def)))
      existing.def.push_back(std::move(def));
  }
}

std::string_view DB::GetSymbolName(SymbolIdx sym, bool qualified) {
  Usr usr = sym.usr;
  switch (sym.kind) {
  default:
    break;
  case SymbolKind::File:
    if (files[usr].def)
      return files[usr].def->path;
    break;
  case SymbolKind::Func:
    if (const auto *def = Func(usr).AnyDef())
      return def->Name(qualified);
    break;
  case SymbolKind::Type:
    if (const auto *def = Type(usr).AnyDef())
      return def->Name(qualified);
    break;
  case SymbolKind::Var:
    if (const auto *def = Var(usr).AnyDef())
      return def->Name(qualified);
    break;
  }
  return "";
}
