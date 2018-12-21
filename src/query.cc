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

#include "query.hh"

#include "indexer.hh"
#include "pipeline.hh"
#include "serializer.hh"

#include <rapidjson/document.h>

#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace ccls {
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

QueryFile::DefUpdate BuildFileDefUpdate(IndexFile &&indexed) {
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
  static IndexFile empty(current->path, "<empty>");
  if (previous)
    r.prev_lid2path = std::move(previous->lid2path);
  else
    previous = &empty;
  r.lid2path = std::move(current->lid2path);

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

  r.files_def_update = BuildFileDefUpdate(std::move(*current));
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
void DB::RemoveUsrs(Kind kind, int file_id,
                    const std::vector<std::pair<Usr, Def>> &to_remove) {
  switch (kind) {
  case Kind::Func: {
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
  case Kind::Type: {
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
  case Kind::Var: {
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
  auto Ref = [&](std::unordered_map<int, int> &lid2fid, Usr usr, Kind kind,
                 Use &use, int delta) {
    use.file_id =
        use.file_id == -1 ? u->file_id : lid2fid.find(use.file_id)->second;
    ExtentRef sym{{use.range, usr, kind, use.role}};
    int &v = files[use.file_id].symbol2refcnt[sym];
    v += delta;
    assert(v >= 0);
    if (!v)
      files[use.file_id].symbol2refcnt.erase(sym);
  };
  auto RefDecl = [&](std::unordered_map<int, int> &lid2fid, Usr usr, Kind kind,
                     DeclRef &dr, int delta) {
    dr.file_id =
        dr.file_id == -1 ? u->file_id : lid2fid.find(dr.file_id)->second;
    ExtentRef sym{{dr.range, usr, kind, dr.role}, dr.extent};
    int &v = files[dr.file_id].symbol2refcnt[sym];
    v += delta;
    assert(v >= 0);
    if (!v)
      files[dr.file_id].symbol2refcnt.erase(sym);
  };

  auto UpdateUses =
      [&](Usr usr, Kind kind,
          llvm::DenseMap<Usr, int, DenseMapInfoForUsr> &entity_usr,
          auto &entities, auto &p, bool hint_implicit) {
        auto R = entity_usr.try_emplace(usr, entity_usr.size());
        if (R.second)
          entities.emplace_back().usr = usr;
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
  for (auto &[usr, def] : u->funcs_removed)
    if (def.spell)
      RefDecl(prev_lid2file_id, usr, Kind::Func, *def.spell, -1);
  RemoveUsrs(Kind::Func, u->file_id, u->funcs_removed);
  Update(lid2file_id, u->file_id, std::move(u->funcs_def_update));
  for (auto &[usr, del_add]: u->funcs_declarations) {
    for (DeclRef &dr : del_add.first)
      RefDecl(prev_lid2file_id, usr, Kind::Func, dr, -1);
    for (DeclRef &dr : del_add.second)
      RefDecl(lid2file_id, usr, Kind::Func, dr, 1);
  }
  REMOVE_ADD(func, declarations);
  REMOVE_ADD(func, derived);
  for (auto &[usr, p] : u->funcs_uses)
    UpdateUses(usr, Kind::Func, func_usr, funcs, p, true);

  if ((t = types.size() + u->types_hint) > types.capacity()) {
    t = size_t(t * grow);
    types.reserve(t);
    type_usr.reserve(t);
  }
  for (auto &[usr, def] : u->types_removed)
    if (def.spell)
      RefDecl(prev_lid2file_id, usr, Kind::Type, *def.spell, -1);
  RemoveUsrs(Kind::Type, u->file_id, u->types_removed);
  Update(lid2file_id, u->file_id, std::move(u->types_def_update));
  for (auto &[usr, del_add]: u->types_declarations) {
    for (DeclRef &dr : del_add.first)
      RefDecl(prev_lid2file_id, usr, Kind::Type, dr, -1);
    for (DeclRef &dr : del_add.second)
      RefDecl(lid2file_id, usr, Kind::Type, dr, 1);
  }
  REMOVE_ADD(type, declarations);
  REMOVE_ADD(type, derived);
  REMOVE_ADD(type, instances);
  for (auto &[usr, p] : u->types_uses)
    UpdateUses(usr, Kind::Type, type_usr, types, p, false);

  if ((t = vars.size() + u->vars_hint) > vars.capacity()) {
    t = size_t(t * grow);
    vars.reserve(t);
    var_usr.reserve(t);
  }
  for (auto &[usr, def] : u->vars_removed)
    if (def.spell)
      RefDecl(prev_lid2file_id, usr, Kind::Var, *def.spell, -1);
  RemoveUsrs(Kind::Var, u->file_id, u->vars_removed);
  Update(lid2file_id, u->file_id, std::move(u->vars_def_update));
  for (auto &[usr, del_add]: u->vars_declarations) {
    for (DeclRef &dr : del_add.first)
      RefDecl(prev_lid2file_id, usr, Kind::Var, dr, -1);
    for (DeclRef &dr : del_add.second)
      RefDecl(lid2file_id, usr, Kind::Var, dr, 1);
  }
  REMOVE_ADD(var, declarations);
  for (auto &[usr, p] : u->vars_uses)
    UpdateUses(usr, Kind::Var, var_usr, vars, p, false);

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
          {def.spell->range, u.first, Kind::Func, def.spell->role},
          def.spell->extent}]++;
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
          {def.spell->range, u.first, Kind::Type, def.spell->role},
          def.spell->extent}]++;
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
          {def.spell->range, u.first, Kind::Var, def.spell->role},
          def.spell->extent}]++;
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
  case Kind::File:
    if (files[usr].def)
      return files[usr].def->path;
    break;
  case Kind::Func:
    if (const auto *def = Func(usr).AnyDef())
      return def->Name(qualified);
    break;
  case Kind::Type:
    if (const auto *def = Type(usr).AnyDef())
      return def->Name(qualified);
    break;
  case Kind::Var:
    if (const auto *def = Var(usr).AnyDef())
      return def->Name(qualified);
    break;
  }
  return "";
}

std::vector<uint8_t> DB::GetFileSet(const std::vector<std::string> &folders) {
  if (folders.empty())
    return std::vector<uint8_t>(files.size(), 1);
  std::vector<uint8_t> file_set(files.size());
  for (QueryFile &file : files)
    if (file.def) {
      bool ok = false;
      for (auto &folder : folders)
        if (llvm::StringRef(file.def->path).startswith(folder)) {
          ok = true;
          break;
        }
      if (ok)
        file_set[file.id] = 1;
    }
  return file_set;
}

namespace {
// Computes roughly how long |range| is.
int ComputeRangeSize(const Range &range) {
  if (range.start.line != range.end.line)
    return INT_MAX;
  return range.end.column - range.start.column;
}

template <typename Q>
std::vector<Use>
GetDeclarations(llvm::DenseMap<Usr, int, DenseMapInfoForUsr> &entity_usr,
                std::vector<Q> &entities, const std::vector<Usr> &usrs) {
  std::vector<Use> ret;
  ret.reserve(usrs.size());
  for (Usr usr : usrs) {
    Q &entity = entities[entity_usr[{usr}]];
    bool has_def = false;
    for (auto &def : entity.def)
      if (def.spell) {
        ret.push_back(*def.spell);
        has_def = true;
        break;
      }
    if (!has_def && entity.declarations.size())
      ret.push_back(entity.declarations[0]);
  }
  return ret;
}
}

Maybe<DeclRef> GetDefinitionSpell(DB *db, SymbolIdx sym) {
  Maybe<DeclRef> ret;
  EachEntityDef(db, sym, [&](const auto &def) { return !(ret = def.spell); });
  return ret;
}

std::vector<Use> GetFuncDeclarations(DB *db, const std::vector<Usr> &usrs) {
  return GetDeclarations(db->func_usr, db->funcs, usrs);
}
std::vector<Use> GetTypeDeclarations(DB *db, const std::vector<Usr> &usrs) {
  return GetDeclarations(db->type_usr, db->types, usrs);
}
std::vector<DeclRef> GetVarDeclarations(DB *db, const std::vector<Usr> &usrs,
                                        unsigned kind) {
  std::vector<DeclRef> ret;
  ret.reserve(usrs.size());
  for (Usr usr : usrs) {
    QueryVar &var = db->Var(usr);
    bool has_def = false;
    for (auto &def : var.def)
      if (def.spell) {
        has_def = true;
        // See messages/ccls_vars.cc
        if (def.kind == SymbolKind::Field) {
          if (!(kind & 1))
            break;
        } else if (def.kind == SymbolKind::Variable) {
          if (!(kind & 2))
            break;
        } else if (def.kind == SymbolKind::Parameter) {
          if (!(kind & 4))
            break;
        }
        ret.push_back(*def.spell);
        break;
      }
    if (!has_def && var.declarations.size())
      ret.push_back(var.declarations[0]);
  }
  return ret;
}

std::vector<DeclRef> &GetNonDefDeclarations(DB *db, SymbolIdx sym) {
  static std::vector<DeclRef> empty;
  switch (sym.kind) {
  case Kind::Func:
    return db->GetFunc(sym).declarations;
  case Kind::Type:
    return db->GetType(sym).declarations;
  case Kind::Var:
    return db->GetVar(sym).declarations;
  default:
    break;
  }
  return empty;
}

std::vector<Use> GetUsesForAllBases(DB *db, QueryFunc &root) {
  std::vector<Use> ret;
  std::vector<QueryFunc *> stack{&root};
  std::unordered_set<Usr> seen;
  seen.insert(root.usr);
  while (!stack.empty()) {
    QueryFunc &func = *stack.back();
    stack.pop_back();
    if (auto *def = func.AnyDef()) {
      EachDefinedFunc(db, def->bases, [&](QueryFunc &func1) {
        if (!seen.count(func1.usr)) {
          seen.insert(func1.usr);
          stack.push_back(&func1);
          ret.insert(ret.end(), func1.uses.begin(), func1.uses.end());
        }
      });
    }
  }

  return ret;
}

std::vector<Use> GetUsesForAllDerived(DB *db, QueryFunc &root) {
  std::vector<Use> ret;
  std::vector<QueryFunc *> stack{&root};
  std::unordered_set<Usr> seen;
  seen.insert(root.usr);
  while (!stack.empty()) {
    QueryFunc &func = *stack.back();
    stack.pop_back();
    EachDefinedFunc(db, func.derived, [&](QueryFunc &func1) {
      if (!seen.count(func1.usr)) {
        seen.insert(func1.usr);
        stack.push_back(&func1);
        ret.insert(ret.end(), func1.uses.begin(), func1.uses.end());
      }
    });
  }

  return ret;
}

std::optional<lsRange> GetLsRange(WorkingFile *wfile,
                                  const Range &location) {
  if (!wfile || wfile->index_lines.empty())
    return lsRange{Position{location.start.line, location.start.column},
                   Position{location.end.line, location.end.column}};

  int start_column = location.start.column, end_column = location.end.column;
  std::optional<int> start = wfile->GetBufferPosFromIndexPos(
      location.start.line, &start_column, false);
  std::optional<int> end = wfile->GetBufferPosFromIndexPos(
      location.end.line, &end_column, true);
  if (!start || !end)
    return std::nullopt;

  // If remapping end fails (end can never be < start), just guess that the
  // final location didn't move. This only screws up the highlighted code
  // region if we guess wrong, so not a big deal.
  //
  // Remapping fails often in C++ since there are a lot of "};" at the end of
  // class/struct definitions.
  if (*end < *start)
    *end = *start + (location.end.line - location.start.line);
  if (*start == *end && start_column > end_column)
    end_column = start_column;

  return lsRange{Position{*start, start_column}, Position{*end, end_column}};
}

DocumentUri GetLsDocumentUri(DB *db, int file_id, std::string *path) {
  QueryFile &file = db->files[file_id];
  if (file.def) {
    *path = file.def->path;
    return DocumentUri::FromPath(*path);
  } else {
    *path = "";
    return DocumentUri::FromPath("");
  }
}

DocumentUri GetLsDocumentUri(DB *db, int file_id) {
  QueryFile &file = db->files[file_id];
  if (file.def) {
    return DocumentUri::FromPath(file.def->path);
  } else {
    return DocumentUri::FromPath("");
  }
}

std::optional<Location> GetLsLocation(DB *db, WorkingFiles *wfiles, Use use) {
  std::string path;
  DocumentUri uri = GetLsDocumentUri(db, use.file_id, &path);
  std::optional<lsRange> range = GetLsRange(wfiles->GetFile(path), use.range);
  if (!range)
    return std::nullopt;
  return Location{uri, *range};
}

std::optional<Location> GetLsLocation(DB *db, WorkingFiles *wfiles,
                                      SymbolRef sym, int file_id) {
  return GetLsLocation(db, wfiles, Use{{sym.range, sym.role}, file_id});
}

LocationLink GetLocationLink(DB *db, WorkingFiles *wfiles, DeclRef dr) {
  std::string path;
  DocumentUri uri = GetLsDocumentUri(db, dr.file_id, &path);
  if (auto range = GetLsRange(wfiles->GetFile(path), dr.range))
    if (auto extent = GetLsRange(wfiles->GetFile(path), dr.extent)) {
      LocationLink ret;
      ret.targetUri = uri.raw_uri;
      ret.targetSelectionRange = *range;
      ret.targetRange = extent->Includes(*range) ? *extent : *range;
      return ret;
    }
  return {};
}

SymbolKind GetSymbolKind(DB *db, SymbolIdx sym) {
  SymbolKind ret;
  if (sym.kind == Kind::File)
    ret = SymbolKind::File;
  else {
    ret = SymbolKind::Unknown;
    WithEntity(db, sym, [&](const auto &entity) {
      for (auto &def : entity.def) {
        ret = def.kind;
        break;
      }
    });
  }
  return ret;
}

std::optional<SymbolInformation> GetSymbolInfo(DB *db, SymbolIdx sym,
                                               bool detailed) {
  switch (sym.kind) {
  case Kind::Invalid:
    break;
  case Kind::File: {
    QueryFile &file = db->GetFile(sym);
    if (!file.def)
      break;

    SymbolInformation info;
    info.name = file.def->path;
    info.kind = SymbolKind::File;
    return info;
  }
  default: {
    SymbolInformation info;
    EachEntityDef(db, sym, [&](const auto &def) {
      if (detailed)
        info.name = def.detailed_name;
      else
        info.name = def.Name(true);
      info.kind = def.kind;
      return false;
    });
    return info;
  }
  }

  return std::nullopt;
}

std::vector<SymbolRef> FindSymbolsAtLocation(WorkingFile *wfile,
                                             QueryFile *file, Position &ls_pos,
                                             bool smallest) {
  std::vector<SymbolRef> symbols;
  // If multiVersion > 0, index may not exist and thus index_lines is empty.
  if (wfile && wfile->index_lines.size()) {
    if (auto line = wfile->GetIndexPosFromBufferPos(
            ls_pos.line, &ls_pos.character, false)) {
      ls_pos.line = *line;
    } else {
      ls_pos.line = -1;
      return {};
    }
  }

  for (auto [sym, refcnt] : file->symbol2refcnt)
    if (refcnt > 0 && sym.range.Contains(ls_pos.line, ls_pos.character))
      symbols.push_back(sym);

  // Order shorter ranges first, since they are more detailed/precise. This is
  // important for macros which generate code so that we can resolving the
  // macro argument takes priority over the entire macro body.
  //
  // Order Kind::Var before Kind::Type. Macro calls are treated as Var
  // currently. If a macro expands to tokens led by a Kind::Type, the macro and
  // the Type have the same range. We want to find the macro definition instead
  // of the Type definition.
  //
  // Then order functions before other types, which makes goto definition work
  // better on constructors.
  std::sort(
      symbols.begin(), symbols.end(),
      [](const SymbolRef &a, const SymbolRef &b) {
        int t = ComputeRangeSize(a.range) - ComputeRangeSize(b.range);
        if (t)
          return t < 0;
        // MacroExpansion
        if ((t = (a.role & Role::Dynamic) - (b.role & Role::Dynamic)))
          return t > 0;
        if ((t = (a.role & Role::Definition) - (b.role & Role::Definition)))
          return t > 0;
        // operator> orders Var/Func before Type.
        t = static_cast<int>(a.kind) - static_cast<int>(b.kind);
        if (t)
          return t > 0;
        return a.usr < b.usr;
      });
  if (symbols.size() && smallest) {
    SymbolRef sym = symbols[0];
    for (size_t i = 1; i < symbols.size(); i++)
      if (!(sym.range == symbols[i].range && sym.kind == symbols[i].kind)) {
        symbols.resize(i);
        break;
      }
  }

  return symbols;
}
} // namespace ccls
