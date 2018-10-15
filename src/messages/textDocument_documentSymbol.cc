// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
using namespace ccls;
using namespace clang;

MAKE_HASHABLE(SymbolIdx, t.usr, t.kind);

namespace {
MethodType kMethodType = "textDocument/documentSymbol";

struct In_TextDocumentDocumentSymbol : public RequestMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct Params {
    lsTextDocumentIdentifier textDocument;
    // false: outline; true: all symbols
    bool all = false;
    // If >= 0, return Range[] instead of SymbolInformation[] to reduce output.
    int startLine = -1;
    int endLine = -1;
  } params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentDocumentSymbol::Params, textDocument, all,
                    startLine, endLine);
MAKE_REFLECT_STRUCT(In_TextDocumentDocumentSymbol, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentDocumentSymbol);

struct lsDocumentSymbol {
  std::string name;
  std::string detail;
  lsSymbolKind kind;
  lsRange range;
  lsRange selectionRange;
  std::vector<std::unique_ptr<lsDocumentSymbol>> children;
};
void Reflect(Writer &vis, std::unique_ptr<lsDocumentSymbol> &v);
MAKE_REFLECT_STRUCT(lsDocumentSymbol, name, detail, kind, range, selectionRange,
                    children);
void Reflect(Writer &vis, std::unique_ptr<lsDocumentSymbol> &v) {
  Reflect(vis, *v);
}

template <typename Def>
bool Ignore(const Def *def) {
  return false;
}
template <>
bool Ignore(const QueryType::Def *def) {
  return !def || def->kind == lsSymbolKind::TypeParameter;
}
template<>
bool Ignore(const QueryVar::Def *def) {
  return !def || def->is_local();
}

struct Handler_TextDocumentDocumentSymbol
    : BaseMessageHandler<In_TextDocumentDocumentSymbol> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_TextDocumentDocumentSymbol *request) override {
    auto &params = request->params;

    QueryFile *file;
    int file_id;
    if (!FindFileOrFail(db, project, request->id,
                        params.textDocument.uri.GetPath(), &file, &file_id))
      return;
    WorkingFile *wfile = working_files->GetFileByFilename(file->def->path);
    if (!wfile)
      return;

    if (params.startLine >= 0) {
      std::vector<lsRange> result;
      for (auto [sym, refcnt] : file->symbol2refcnt)
        if (refcnt > 0 && (params.all || sym.extent.Valid()) &&
            params.startLine <= sym.range.start.line &&
            sym.range.start.line <= params.endLine)
          if (auto loc = GetLsLocation(db, working_files, sym, file_id))
            result.push_back(loc->range);
      std::sort(result.begin(), result.end());
      pipeline::Reply(request->id, result);
    } else if (g_config->client.hierarchicalDocumentSymbolSupport) {
      std::unordered_map<SymbolIdx, std::unique_ptr<lsDocumentSymbol>> sym2ds;
      std::vector<std::pair<std::vector<const void *>, lsDocumentSymbol *>>
          funcs, types;
      for (auto [sym, refcnt] : file->symbol2refcnt) {
        if (refcnt <= 0 || !sym.extent.Valid())
          continue;
        auto r = sym2ds.try_emplace(SymbolIdx{sym.usr, sym.kind});
        if (!r.second)
          continue;
        auto &ds = r.first->second;
        ds = std::make_unique<lsDocumentSymbol>();
        std::vector<const void *> def_ptrs;
        WithEntity(db, sym, [&, sym = sym](const auto &entity) {
          auto *def = entity.AnyDef();
          if (!def)
            return;
          ds->name = def->Name(false);
          ds->detail = def->Name(true);
          if (auto ls_range = GetLsRange(wfile, sym.range)) {
            ds->selectionRange = *ls_range;
            ds->range = ds->selectionRange;
            if (sym.extent.Valid())
              if (auto ls_range1 = GetLsRange(wfile, sym.extent))
                ds->range = *ls_range1;
          }

          for (auto &def : entity.def)
            if (def.file_id == file_id && !Ignore(&def)) {
              ds->kind = def.kind;
              if (def.spell || def.kind == lsSymbolKind::Namespace)
                def_ptrs.push_back(&def);
            }
        });
        if (def_ptrs.empty() || !(params.all || sym.role & Role::Definition ||
                                  ds->kind == lsSymbolKind::Namespace)) {
          ds.reset();
          continue;
        }
        if (sym.kind == SymbolKind::Func)
          funcs.emplace_back(std::move(def_ptrs), ds.get());
        else if (sym.kind == SymbolKind::Type)
          types.emplace_back(std::move(def_ptrs), ds.get());
      }

      for (auto &[def_ptrs, ds] : funcs)
        for (const void *def_ptr : def_ptrs)
          for (Usr usr1 : ((const QueryFunc::Def *)def_ptr)->vars) {
            auto it = sym2ds.find(SymbolIdx{usr1, SymbolKind::Var});
            if (it != sym2ds.end() && it->second)
              ds->children.push_back(std::move(it->second));
          }
      for (auto &[def_ptrs, ds] : types)
        for (const void *def_ptr : def_ptrs) {
          auto *def = (const QueryType::Def *)def_ptr;
          for (Usr usr1 : def->funcs) {
            auto it = sym2ds.find(SymbolIdx{usr1, SymbolKind::Func});
            if (it != sym2ds.end() && it->second)
              ds->children.push_back(std::move(it->second));
          }
          for (Usr usr1 : def->types) {
            auto it = sym2ds.find(SymbolIdx{usr1, SymbolKind::Type});
            if (it != sym2ds.end() && it->second)
              ds->children.push_back(std::move(it->second));
          }
          for (auto [usr1, _] : def->vars) {
            auto it = sym2ds.find(SymbolIdx{usr1, SymbolKind::Var});
            if (it != sym2ds.end() && it->second)
              ds->children.push_back(std::move(it->second));
          }
        }
      std::vector<std::unique_ptr<lsDocumentSymbol>> result;
      for (auto &[_, ds] : sym2ds)
        if (ds)
          result.push_back(std::move(ds));
      pipeline::Reply(request->id, result);
    } else {
      std::vector<lsSymbolInformation> result;
      for (auto [sym, refcnt] : file->symbol2refcnt) {
        if (refcnt <= 0 || !sym.extent.Valid() ||
            !(params.all || sym.role & Role::Definition))
          continue;
        if (std::optional<lsSymbolInformation> info =
                GetSymbolInfo(db, sym, false)) {
          if ((sym.kind == SymbolKind::Type &&
               Ignore(db->GetType(sym).AnyDef())) ||
              (sym.kind == SymbolKind::Var &&
               Ignore(db->GetVar(sym).AnyDef())))
            continue;
          if (auto loc = GetLsLocation(db, working_files, sym, file_id)) {
            info->location = *loc;
            result.push_back(*info);
          }
        }
      }
      pipeline::Reply(request->id, result);
    }
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDocumentSymbol);
} // namespace
