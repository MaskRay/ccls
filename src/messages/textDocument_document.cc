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

#include "message_handler.hh"
#include "pipeline.hh"
#include "query_utils.hh"

#include <algorithm>

MAKE_HASHABLE(ccls::SymbolIdx, t.usr, t.kind);

namespace ccls {
MAKE_REFLECT_STRUCT(SymbolInformation, name, kind, location, containerName);

namespace {
struct DocumentHighlight {
  enum Kind { Text = 1, Read = 2, Write = 3 };

  lsRange range;
  int kind = 1;

  // ccls extension
  Role role = Role::None;

  bool operator<(const DocumentHighlight &o) const {
    return !(range == o.range) ? range < o.range : kind < o.kind;
  }
};
MAKE_REFLECT_STRUCT(DocumentHighlight, range, kind, role);
} // namespace

void MessageHandler::textDocument_documentHighlight(
    TextDocumentPositionParam &param, ReplyOnce &reply) {
  int file_id;
  QueryFile *file = FindFile(reply, param.textDocument.uri.GetPath(), &file_id);
  if (!file)
    return;

  WorkingFile *wfile = wfiles->GetFileByFilename(file->def->path);
  std::vector<DocumentHighlight> result;

  std::vector<SymbolRef> syms =
      FindSymbolsAtLocation(wfile, file, param.position, true);
  for (auto [sym, refcnt] : file->symbol2refcnt) {
    if (refcnt <= 0)
      continue;
    Usr usr = sym.usr;
    Kind kind = sym.kind;
    if (std::none_of(syms.begin(), syms.end(), [&](auto &sym1) {
          return usr == sym1.usr && kind == sym1.kind;
        }))
      continue;
    if (auto loc = GetLsLocation(db, wfiles, sym, file_id)) {
      DocumentHighlight highlight;
      highlight.range = loc->range;
      if (sym.role & Role::Write)
        highlight.kind = DocumentHighlight::Write;
      else if (sym.role & Role::Read)
        highlight.kind = DocumentHighlight::Read;
      else
        highlight.kind = DocumentHighlight::Text;
      highlight.role = sym.role;
      result.push_back(highlight);
    }
  }
  std::sort(result.begin(), result.end());
  reply(result);
}

namespace {
struct DocumentLink {
  lsRange range;
  DocumentUri target;
};
MAKE_REFLECT_STRUCT(DocumentLink, range, target);
} // namespace

void MessageHandler::textDocument_documentLink(TextDocumentParam &param,
                                               ReplyOnce &reply) {
  QueryFile *file = FindFile(reply, param.textDocument.uri.GetPath());
  if (!file)
    return;

  std::vector<DocumentLink> result;
  for (const IndexInclude &include : file->def->includes)
    result.push_back({lsRange{{include.line, 0}, {include.line + 1, 0}},
                      DocumentUri::FromPath(include.resolved_path)});
  reply(result);
} // namespace ccls

namespace {
struct DocumentSymbolParam : TextDocumentParam {
  // false: outline; true: all symbols
  bool all = false;
  // If >= 0, return Range[] instead of SymbolInformation[] to reduce output.
  int startLine = -1;
  int endLine = -1;
};
MAKE_REFLECT_STRUCT(DocumentSymbolParam, textDocument, all, startLine, endLine);

struct DocumentSymbol {
  std::string name;
  std::string detail;
  SymbolKind kind;
  lsRange range;
  lsRange selectionRange;
  std::vector<std::unique_ptr<DocumentSymbol>> children;
};
void Reflect(Writer &vis, std::unique_ptr<DocumentSymbol> &v);
MAKE_REFLECT_STRUCT(DocumentSymbol, name, detail, kind, range, selectionRange,
                    children);
void Reflect(Writer &vis, std::unique_ptr<DocumentSymbol> &v) {
  Reflect(vis, *v);
}

template <typename Def>
bool Ignore(const Def *def) {
  return false;
}
template <>
bool Ignore(const QueryType::Def *def) {
  return !def || def->kind == SymbolKind::TypeParameter;
}
template<>
bool Ignore(const QueryVar::Def *def) {
  return !def || def->is_local();
}
} // namespace

void MessageHandler::textDocument_documentSymbol(Reader &reader,
                                                 ReplyOnce &reply) {
  DocumentSymbolParam param;
  Reflect(reader, param);

  int file_id;
  QueryFile *file = FindFile(reply, param.textDocument.uri.GetPath(), &file_id);
  if (!file)
    return;
  WorkingFile *wfile = wfiles->GetFileByFilename(file->def->path);
  if (!wfile)
    return;

  if (param.startLine >= 0) {
    std::vector<lsRange> result;
    for (auto [sym, refcnt] : file->symbol2refcnt)
      if (refcnt > 0 && (param.all || sym.extent.Valid()) &&
          param.startLine <= sym.range.start.line &&
          sym.range.start.line <= param.endLine)
        if (auto loc = GetLsLocation(db, wfiles, sym, file_id))
          result.push_back(loc->range);
    std::sort(result.begin(), result.end());
    reply(result);
  } else if (g_config->client.hierarchicalDocumentSymbolSupport) {
    std::unordered_map<SymbolIdx, std::unique_ptr<DocumentSymbol>> sym2ds;
    std::vector<std::pair<std::vector<const void *>, DocumentSymbol *>> funcs,
        types;
    for (auto [sym, refcnt] : file->symbol2refcnt) {
      if (refcnt <= 0 || !sym.extent.Valid())
        continue;
      auto r = sym2ds.try_emplace(SymbolIdx{sym.usr, sym.kind});
      if (!r.second)
        continue;
      auto &ds = r.first->second;
      ds = std::make_unique<DocumentSymbol>();
      if (auto range = GetLsRange(wfile, sym.range)) {
        ds->selectionRange = *range;
        ds->range = ds->selectionRange;
        if (sym.extent.Valid())
          if (auto range1 = GetLsRange(wfile, sym.extent))
            ds->range = *range1;
      }
      std::vector<const void *> def_ptrs;
      WithEntity(db, sym, [&](const auto &entity) {
        auto *def = entity.AnyDef();
        if (!def)
          return;
        ds->name = def->Name(false);
        ds->detail = def->Name(true);
        for (auto &def : entity.def)
          if (def.file_id == file_id && !Ignore(&def)) {
            ds->kind = def.kind;
            if (def.spell || def.kind == SymbolKind::Namespace)
              def_ptrs.push_back(&def);
          }
      });
      if (!(param.all || sym.role & Role::Definition ||
            ds->kind == SymbolKind::Method ||
            ds->kind == SymbolKind::Namespace)) {
        ds.reset();
        continue;
      }
      if (def_ptrs.empty())
        continue;
      if (sym.kind == Kind::Func)
        funcs.emplace_back(std::move(def_ptrs), ds.get());
      else if (sym.kind == Kind::Type)
        types.emplace_back(std::move(def_ptrs), ds.get());
    }

    for (auto &[def_ptrs, ds] : funcs)
      for (const void *def_ptr : def_ptrs)
        for (Usr usr1 : ((const QueryFunc::Def *)def_ptr)->vars) {
          auto it = sym2ds.find(SymbolIdx{usr1, Kind::Var});
          if (it != sym2ds.end() && it->second)
            ds->children.push_back(std::move(it->second));
        }
    for (auto &[def_ptrs, ds] : types)
      for (const void *def_ptr : def_ptrs) {
        auto *def = (const QueryType::Def *)def_ptr;
        for (Usr usr1 : def->funcs) {
          auto it = sym2ds.find(SymbolIdx{usr1, Kind::Func});
          if (it != sym2ds.end() && it->second)
            ds->children.push_back(std::move(it->second));
        }
        for (Usr usr1 : def->types) {
          auto it = sym2ds.find(SymbolIdx{usr1, Kind::Type});
          if (it != sym2ds.end() && it->second)
            ds->children.push_back(std::move(it->second));
        }
        for (auto [usr1, _] : def->vars) {
          auto it = sym2ds.find(SymbolIdx{usr1, Kind::Var});
          if (it != sym2ds.end() && it->second)
            ds->children.push_back(std::move(it->second));
        }
      }
    std::vector<std::unique_ptr<DocumentSymbol>> result;
    for (auto &[_, ds] : sym2ds)
      if (ds)
        result.push_back(std::move(ds));
    reply(result);
  } else {
    std::vector<SymbolInformation> result;
    for (auto [sym, refcnt] : file->symbol2refcnt) {
      if (refcnt <= 0 || !sym.extent.Valid() ||
          !(param.all || sym.role & Role::Definition))
        continue;
      if (std::optional<SymbolInformation> info =
              GetSymbolInfo(db, sym, false)) {
        if ((sym.kind == Kind::Type && Ignore(db->GetType(sym).AnyDef())) ||
            (sym.kind == Kind::Var && Ignore(db->GetVar(sym).AnyDef())))
          continue;
        if (auto loc = GetLsLocation(db, wfiles, sym, file_id)) {
          info->location = *loc;
          result.push_back(*info);
        }
      }
    }
    reply(result);
  }
}
} // namespace ccls
