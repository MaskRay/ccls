/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

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

void AssignFileId(const Lid2file_id &lid2file_id, int file_id, Use &use) {
  if (use.file_id == -1)
    use.file_id = file_id;
  else
    use.file_id = lid2file_id.find(use.file_id)->second;
}

template <typename T>
void AddRange(std::vector<T> &into, const std::vector<T> &from) {
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
    def.dependencies.push_back(dep.first.val().data()); // llvm 8 -> data()
  def.language = indexed.language;
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

  r.funcs_hint = current->usr2func.size() - previous->usr2func.size();
  for (auto &it : previous->usr2func) {
    auto &func = it.second;
    if (func.def.detailed_name[0])
      r.funcs_removed.emplace_back(func.usr, func.def);
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

  r.types_hint = current->usr2type.size() - previous->usr2type.size();
  for (auto &it : previous->usr2type) {
    auto &type = it.second;
    if (type.def.detailed_name[0])
      r.types_removed.emplace_back(type.usr, type.def);
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

  r.vars_hint = current->usr2var.size() - previous->usr2var.size();
  for (auto &it : previous->usr2var) {
    auto &var = it.second;
    if (var.def.detailed_name[0])
      r.vars_removed.emplace_back(var.usr, var.def);
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

void DB::clear() {
  files.clear();
  name2file_id.clear();
  func_usr.clear();
  type_usr.clear();
  var_usr.clear();
  funcs.clear();
  types.clear();
  vars.clear();
}

template <typename Def>
void DB::RemoveUsrs(SymbolKind kind, int file_id,
                    const std::vector<std::pair<Usr, Def>> &to_remove) {
  switch (kind) {
  case SymbolKind::Func: {
    for (auto &[usr, _] : to_remove) {
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
    for (auto &[usr, _] : to_remove) {
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
    for (auto &[usr, _] : to_remove) {
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
    RemoveRange(entity.F, it.second.first);                                    \
    AddRange(entity.F, it.second.second);                                      \
  }

  std::unordered_map<int, int> prev_lid2file_id, lid2file_id;
  for (auto &[lid, path] : u->prev_lid2path)
    prev_lid2file_id[lid] = GetFileId(path);
  for (auto &[lid, path] : u->lid2path) {
    int file_id = GetFileId(path);
    lid2file_id[lid] = file_id;
    if (!files[file_id].def) {
      files[file_id].def = QueryFile::Def();
      files[file_id].def->path = path;
    }
  }

  // References (Use &use) in this function are important to update file_id.
  auto Ref = [&](std::unordered_map<int, int> &lid2fid, Usr usr,
                 SymbolKind kind, Use &use, int delta, int k = 1) {
    use.file_id =
        use.file_id == -1 ? u->file_id : lid2fid.find(use.file_id)->second;
    SymbolRef sym{use.range, usr, kind, use.role};
    if (k & 1) {
      int &v = files[use.file_id].symbol2refcnt[sym];
      v += delta;
      assert(v >= 0);
      if (!v)
        files[use.file_id].symbol2refcnt.erase(sym);
    }
    if (k & 2) {
      int &v = files[use.file_id].outline2refcnt[sym];
      v += delta;
      assert(v >= 0);
      if (!v)
        files[use.file_id].outline2refcnt.erase(sym);
    }
  };
  auto RefDecl = [&](std::unordered_map<int, int> &lid2fid, Usr usr,
                     SymbolKind kind, DeclRef &dr, int delta) {
    Ref(lid2fid, usr, kind, dr, delta, 1);
    files[dr.file_id]
        .outline2refcnt[SymbolRef{dr.extent, usr, kind, dr.role}] += delta;
  };

  auto UpdateUses =
      [&](Usr usr, SymbolKind kind,
          llvm::DenseMap<Usr, int, DenseMapInfoForUsr> &entity_usr,
          auto &entities, auto &p, bool hint_implicit) {
        auto R = entity_usr.try_emplace(usr, entity_usr.size());
        if (R.second)
          vars.emplace_back().usr = usr;
        auto &entity = entities[R.first->second];
        for (Use &use : p.first) {
          if (hint_implicit && use.role & Role::Implicit) {
            // Make ranges of implicit function calls larger (spanning one more
            // column to the left/right). This is hacky but useful. e.g.
            // textDocument/definition on the space/semicolon in `A a;` or `
            // 42;` will take you to the constructor.
            if (use.range.start.column > 0)
              use.range.start.column--;
            use.range.end.column++;
          }
          Ref(prev_lid2file_id, usr, kind, use, -1);
        }
        RemoveRange(entity.uses, p.first);
        for (Use &use : p.second) {
          if (hint_implicit && use.role & Role::Implicit) {
            if (use.range.start.column > 0)
              use.range.start.column--;
            use.range.end.column++;
          }
          Ref(lid2file_id, usr, kind, use, 1);
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
  for (auto &[usr, def] : u->funcs_removed) {
    if (def.spell)
      Ref(prev_lid2file_id, usr, SymbolKind::Func, *def.spell, -1);
    if (def.extent)
      Ref(prev_lid2file_id, usr, SymbolKind::Func, *def.extent, -1, 2);
  }
  RemoveUsrs(SymbolKind::Func, u->file_id, u->funcs_removed);
  Update(lid2file_id, u->file_id, std::move(u->funcs_def_update));
  for (auto &[usr, del_add]: u->funcs_declarations) {
    for (DeclRef &dr : del_add.first)
      RefDecl(prev_lid2file_id, usr, SymbolKind::Func, dr, -1);
    for (DeclRef &dr : del_add.second)
      RefDecl(lid2file_id, usr, SymbolKind::Func, dr, 1);
  }
  REMOVE_ADD(func, declarations);
  REMOVE_ADD(func, derived);
  for (auto &[usr, p] : u->funcs_uses)
    UpdateUses(usr, SymbolKind::Func, func_usr, funcs, p, true);

  if ((t = types.size() + u->types_hint) > types.capacity()) {
    t = size_t(t * grow);
    types.reserve(t);
    type_usr.reserve(t);
  }
  for (auto &[usr, def] : u->types_removed) {
    if (def.spell)
      Ref(prev_lid2file_id, usr, SymbolKind::Type, *def.spell, -1);
    if (def.extent)
      Ref(prev_lid2file_id, usr, SymbolKind::Type, *def.extent, -1, 2);
  }
  RemoveUsrs(SymbolKind::Type, u->file_id, u->types_removed);
  Update(lid2file_id, u->file_id, std::move(u->types_def_update));
  for (auto &[usr, del_add]: u->types_declarations) {
    for (DeclRef &dr : del_add.first)
      RefDecl(prev_lid2file_id, usr, SymbolKind::Type, dr, -1);
    for (DeclRef &dr : del_add.second)
      RefDecl(lid2file_id, usr, SymbolKind::Type, dr, 1);
  }
  REMOVE_ADD(type, declarations);
  REMOVE_ADD(type, derived);
  REMOVE_ADD(type, instances);
  for (auto &[usr, p] : u->types_uses)
    UpdateUses(usr, SymbolKind::Type, type_usr, types, p, false);

  if ((t = vars.size() + u->vars_hint) > vars.capacity()) {
    t = size_t(t * grow);
    vars.reserve(t);
    var_usr.reserve(t);
  }
  for (auto &[usr, def] : u->vars_removed) {
    if (def.spell)
      Ref(prev_lid2file_id, usr, SymbolKind::Var, *def.spell, -1);
    if (def.extent)
      Ref(prev_lid2file_id, usr, SymbolKind::Var, *def.extent, -1, 2);
  }
  RemoveUsrs(SymbolKind::Var, u->file_id, u->vars_removed);
  Update(lid2file_id, u->file_id, std::move(u->vars_def_update));
  for (auto &[usr, del_add]: u->vars_declarations) {
    for (DeclRef &dr : del_add.first)
      RefDecl(prev_lid2file_id, usr, SymbolKind::Var, dr, -1);
    for (DeclRef &dr : del_add.second)
      RefDecl(lid2file_id, usr, SymbolKind::Var, dr, 1);
  }
  REMOVE_ADD(var, declarations);
  for (auto &[usr, p] : u->vars_uses)
    UpdateUses(usr, SymbolKind::Var, var_usr, vars, p, false);

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
    if (def.spell) {
      AssignFileId(lid2file_id, file_id, *def.spell);
      files[def.spell->file_id].symbol2refcnt[{
          def.spell->range, u.first, SymbolKind::Func, def.spell->role}]++;
    }
    if (def.extent) {
      AssignFileId(lid2file_id, file_id, *def.extent);
      files[def.extent->file_id].outline2refcnt[{
          def.extent->range, u.first, SymbolKind::Func, def.extent->role}]++;
    }

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
    if (def.spell) {
      AssignFileId(lid2file_id, file_id, *def.spell);
      files[def.spell->file_id].symbol2refcnt[{
          def.spell->range, u.first, SymbolKind::Type, def.spell->role}]++;
    }
    if (def.extent) {
      AssignFileId(lid2file_id, file_id, *def.extent);
      files[def.extent->file_id].outline2refcnt[{
          def.extent->range, u.first, SymbolKind::Type, def.extent->role}]++;
    }
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
    if (def.spell) {
      AssignFileId(lid2file_id, file_id, *def.spell);
      files[def.spell->file_id].symbol2refcnt[{
          def.spell->range, u.first, SymbolKind::Var, def.spell->role}]++;
    }
    if (def.extent) {
      AssignFileId(lid2file_id, file_id, *def.extent);
      files[def.extent->file_id].outline2refcnt[{
          def.extent->range, u.first, SymbolKind::Var, def.extent->role}]++;
    }
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
