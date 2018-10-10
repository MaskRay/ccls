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

struct In_TextDocumentDocumentSymbol : public RequestInMessage {
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

struct Out_SimpleDocumentSymbol
    : public lsOutMessage<Out_SimpleDocumentSymbol> {
  lsRequestId id;
  std::vector<lsRange> result;
};
MAKE_REFLECT_STRUCT(Out_SimpleDocumentSymbol, jsonrpc, id, result);

struct Out_TextDocumentDocumentSymbol
    : public lsOutMessage<Out_TextDocumentDocumentSymbol> {
  lsRequestId id;
  std::vector<lsSymbolInformation> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentDocumentSymbol, jsonrpc, id, result);

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

struct Out_HierarchicalDocumentSymbol
    : public lsOutMessage<Out_HierarchicalDocumentSymbol> {
  lsRequestId id;
  std::vector<std::unique_ptr<lsDocumentSymbol>> result;
};
MAKE_REFLECT_STRUCT(Out_HierarchicalDocumentSymbol, jsonrpc, id, result);

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

    const auto &symbol2refcnt =
        params.all ? file->symbol2refcnt : file->outline2refcnt;
    if (params.startLine >= 0) {
      Out_SimpleDocumentSymbol out;
      out.id = request->id;
      for (auto [sym, refcnt] : symbol2refcnt)
        if (refcnt > 0 && params.startLine <= sym.range.start.line &&
            sym.range.start.line <= params.endLine)
          if (auto loc = GetLsLocation(db, working_files, sym, file_id))
            out.result.push_back(loc->range);
      std::sort(out.result.begin(), out.result.end());
      pipeline::WriteStdout(kMethodType, out);
    } else if (g_config->client.hierarchicalDocumentSymbolSupport) {
      std::unordered_map<SymbolIdx, std::unique_ptr<lsDocumentSymbol>> sym2ds;
      std::vector<std::pair<const QueryFunc::Def *, lsDocumentSymbol *>> funcs;
      std::vector<std::pair<const QueryType::Def *, lsDocumentSymbol *>> types;
      for (auto [sym, refcnt] : symbol2refcnt) {
        if (refcnt <= 0)
          continue;
        auto r = sym2ds.try_emplace(SymbolIdx{sym.usr, sym.kind});
        if (!r.second)
          continue;
        auto &ds = r.first->second;
        ds = std::make_unique<lsDocumentSymbol>();
        const void *def_ptr = nullptr;
        WithEntity(db, sym, [&](const auto &entity) {
          auto *def = entity.AnyDef();
          if (!def)
            return;
          ds->name = def->Name(false);
          ds->detail = def->Name(true);

          // Try to find a definition with spell first.
          const void *candidate_def_ptr = nullptr;
          for (auto &def : entity.def)
            if (def.file_id == file_id && !Ignore(&def)) {
              ds->kind = def.kind;
              if (def.kind == lsSymbolKind::Namespace)
                candidate_def_ptr = &def;

              if (!def.spell)
                continue;
              if (auto ls_range = GetLsRange(wfile, def.spell->extent))
                ds->range = *ls_range;
              else
                continue;
              if (auto ls_range = GetLsRange(wfile, def.spell->range))
                ds->selectionRange = *ls_range;
              else
                continue;
              def_ptr = &def;
              break;
            }

          // Try to find a declaration.
          if (!def_ptr && candidate_def_ptr)
            for (auto &decl : entity.declarations)
              if (decl.file_id == file_id) {
                if (auto ls_range = GetLsRange(wfile, decl.extent))
                  ds->range = *ls_range;
                else
                  continue;
                if (auto ls_range = GetLsRange(wfile, decl.range))
                  ds->selectionRange = *ls_range;
                else
                  continue;
                def_ptr = candidate_def_ptr;
                break;
              }
        });
        if (!def_ptr) {
          ds.reset();
          continue;
        }
        if (sym.kind == SymbolKind::Func)
          funcs.emplace_back((const QueryFunc::Def *)def_ptr, ds.get());
        else if (sym.kind == SymbolKind::Type)
          types.emplace_back((const QueryType::Def *)def_ptr, ds.get());
      }

      for (auto &[def, ds] : funcs)
        for (Usr usr1 : def->vars) {
          auto it = sym2ds.find(SymbolIdx{usr1, SymbolKind::Var});
          if (it != sym2ds.end() && it->second)
            ds->children.push_back(std::move(it->second));
        }
      for (auto &[def, ds] : types) {
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
      Out_HierarchicalDocumentSymbol out;
      out.id = request->id;
      for (auto &[_, ds] : sym2ds)
        if (ds)
          out.result.push_back(std::move(ds));
      pipeline::WriteStdout(kMethodType, out);
    } else {
      Out_TextDocumentDocumentSymbol out;
      out.id = request->id;
      for (auto [sym, refcnt] : symbol2refcnt) {
        if (refcnt <= 0) continue;
        if (std::optional<lsSymbolInformation> info =
                GetSymbolInfo(db, sym, false)) {
          if ((sym.kind == SymbolKind::Type &&
               Ignore(db->GetType(sym).AnyDef())) ||
              (sym.kind == SymbolKind::Var &&
               Ignore(db->GetVar(sym).AnyDef())))
            continue;
          if (auto loc = GetLsLocation(db, working_files, sym, file_id)) {
            info->location = *loc;
            out.result.push_back(*info);
          }
        }
      }
      pipeline::WriteStdout(kMethodType, out);
    }
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDocumentSymbol);
} // namespace
