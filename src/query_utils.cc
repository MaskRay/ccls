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

#include "query_utils.h"

#include "pipeline.hh"

#include <limits.h>
#include <unordered_set>

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

} // namespace

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
std::vector<Use> GetVarDeclarations(DB *db, const std::vector<Usr> &usrs,
                                    unsigned kind) {
  std::vector<Use> ret;
  ret.reserve(usrs.size());
  for (Usr usr : usrs) {
    QueryVar &var = db->Var(usr);
    bool has_def = false;
    for (auto &def : var.def)
      if (def.spell) {
        has_def = true;
        // See messages/ccls_vars.cc
        if (def.kind == lsSymbolKind::Field) {
          if (!(kind & 1))
            break;
        } else if (def.kind == lsSymbolKind::Variable) {
          if (!(kind & 2))
            break;
        } else if (def.kind == lsSymbolKind::Parameter) {
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

std::vector<Use> GetNonDefDeclarations(DB *db, SymbolIdx sym) {
  std::vector<Use> ret;
  switch (sym.kind) {
  case SymbolKind::Func:
    for (auto &d : db->GetFunc(sym).declarations)
      ret.push_back(d);
    break;
  case SymbolKind::Type:
    for (auto &d : db->GetType(sym).declarations)
      ret.push_back(d);
    break;
  case SymbolKind::Var:
    for (auto &d : db->GetVar(sym).declarations)
      ret.push_back(d);
    break;
  default:
    break;
  }
  return ret;
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
    return lsRange{lsPosition{location.start.line, location.start.column},
                   lsPosition{location.end.line, location.end.column}};

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

  return lsRange{lsPosition{*start, start_column},
                 lsPosition{*end, end_column}};
}

lsDocumentUri GetLsDocumentUri(DB *db, int file_id, std::string *path) {
  QueryFile &file = db->files[file_id];
  if (file.def) {
    *path = file.def->path;
    return lsDocumentUri::FromPath(*path);
  } else {
    *path = "";
    return lsDocumentUri::FromPath("");
  }
}

lsDocumentUri GetLsDocumentUri(DB *db, int file_id) {
  QueryFile &file = db->files[file_id];
  if (file.def) {
    return lsDocumentUri::FromPath(file.def->path);
  } else {
    return lsDocumentUri::FromPath("");
  }
}

std::optional<lsLocation> GetLsLocation(DB *db, WorkingFiles *wfiles,
                                        Use use) {
  std::string path;
  lsDocumentUri uri = GetLsDocumentUri(db, use.file_id, &path);
  std::optional<lsRange> range =
      GetLsRange(wfiles->GetFileByFilename(path), use.range);
  if (!range)
    return std::nullopt;
  return lsLocation{uri, *range};
}

std::optional<lsLocation> GetLsLocation(DB *db, WorkingFiles *wfiles,
                                        SymbolRef sym, int file_id) {
  return GetLsLocation(db, wfiles, Use{{sym.range, sym.role}, file_id});
}

std::vector<lsLocation> GetLsLocations(DB *db, WorkingFiles *wfiles,
                                       const std::vector<Use> &uses) {
  std::vector<lsLocation> ret;
  for (Use use : uses)
    if (auto loc = GetLsLocation(db, wfiles, use))
      ret.push_back(*loc);
  std::sort(ret.begin(), ret.end());
  ret.erase(std::unique(ret.begin(), ret.end()), ret.end());
  if (ret.size() > g_config->xref.maxNum)
    ret.resize(g_config->xref.maxNum);
  return ret;
}

lsSymbolKind GetSymbolKind(DB *db, SymbolIdx sym) {
  lsSymbolKind ret;
  if (sym.kind == SymbolKind::File)
    ret = lsSymbolKind::File;
  else {
    ret = lsSymbolKind::Unknown;
    WithEntity(db, sym, [&](const auto &entity) {
      for (auto &def : entity.def) {
        ret = def.kind;
        break;
      }
    });
  }
  return ret;
}

std::optional<lsSymbolInformation> GetSymbolInfo(DB *db, SymbolIdx sym,
                                                 bool detailed) {
  switch (sym.kind) {
  case SymbolKind::Invalid:
    break;
  case SymbolKind::File: {
    QueryFile &file = db->GetFile(sym);
    if (!file.def)
      break;

    lsSymbolInformation info;
    info.name = file.def->path;
    info.kind = lsSymbolKind::File;
    return info;
  }
  default: {
    lsSymbolInformation info;
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
                                             QueryFile *file,
                                             lsPosition &ls_pos,
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
  // Order SymbolKind::Var before SymbolKind::Type. Macro calls are treated as
  // Var currently. If a macro expands to tokens led by a SymbolKind::Type, the
  // macro and the Type have the same range. We want to find the macro
  // definition instead of the Type definition.
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
