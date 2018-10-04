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

bool IgnoreType(const QueryType::Def *def) {
  return !def || def->kind == lsSymbolKind::TypeParameter;
}

bool IgnoreVar(const QueryVar::Def *def) {
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
      std::unordered_map<
          SymbolIdx, std::pair<const void *, std::unique_ptr<lsDocumentSymbol>>>
          sym2ds;
      for (auto [sym, refcnt] : symbol2refcnt) {
        if (refcnt <= 0)
          continue;
        auto r = sym2ds.try_emplace(SymbolIdx{sym.usr, sym.kind});
        if (!r.second)
          continue;
        auto &kv = r.first->second;
        kv.second = std::make_unique<lsDocumentSymbol>();
        lsDocumentSymbol &ds = *kv.second;
        WithEntity(db, sym, [&](const auto &entity) {
          auto *def = entity.AnyDef();
          if (!def)
            return;
          ds.name = def->Name(false);
          ds.detail = def->Name(true);
          for (auto &def : entity.def)
            if (def.file_id == file_id) {
              if (!def.spell)
                break;
              ds.kind = def.kind;
              if (auto ls_range = GetLsRange(wfile, def.spell->extent))
                ds.range = *ls_range;
              else
                break;
              if (auto ls_range = GetLsRange(wfile, def.spell->range))
                ds.selectionRange = *ls_range;
              else
                break;
              kv.first = static_cast<const void *>(&def);
            }
        });
        if (kv.first && ((sym.kind == SymbolKind::Type &&
                          IgnoreType((const QueryType::Def *)kv.first)) ||
                         (sym.kind == SymbolKind::Var &&
                          IgnoreVar((const QueryVar::Def *)kv.first))))
          kv.first = nullptr;
        if (!kv.first) {
          kv.second.reset();
          continue;
        }
      }
      for (auto &[sym, def_ds] : sym2ds) {
        if (!def_ds.second)
          continue;
        lsDocumentSymbol &ds = *def_ds.second;
        switch (sym.kind) {
        case SymbolKind::Func: {
          auto &def = *static_cast<const QueryFunc::Def *>(def_ds.first);
          for (Usr usr1 : def.vars) {
            auto it = sym2ds.find(SymbolIdx{usr1, SymbolKind::Var});
            if (it != sym2ds.end() && it->second.second)
              ds.children.push_back(std::move(it->second.second));
          }
          break;
        }
        case SymbolKind::Type: {
          auto &def = *static_cast<const QueryType::Def *>(def_ds.first);
          for (Usr usr1 : def.funcs) {
            auto it = sym2ds.find(SymbolIdx{usr1, SymbolKind::Func});
            if (it != sym2ds.end() && it->second.second)
              ds.children.push_back(std::move(it->second.second));
          }
          for (Usr usr1 : def.types) {
            auto it = sym2ds.find(SymbolIdx{usr1, SymbolKind::Type});
            if (it != sym2ds.end() && it->second.second)
              ds.children.push_back(std::move(it->second.second));
          }
          for (auto [usr1, _] : def.vars) {
            auto it = sym2ds.find(SymbolIdx{usr1, SymbolKind::Var});
            if (it != sym2ds.end() && it->second.second)
              ds.children.push_back(std::move(it->second.second));
          }
          break;
        }
        default:
          break;
        }
      }
      Out_HierarchicalDocumentSymbol out;
      out.id = request->id;
      for (auto &[sym, def_ds] : sym2ds)
        if (def_ds.second)
          out.result.push_back(std::move(def_ds.second));
      pipeline::WriteStdout(kMethodType, out);
    } else {
      Out_TextDocumentDocumentSymbol out;
      out.id = request->id;
      for (auto [sym, refcnt] : symbol2refcnt) {
        if (refcnt <= 0) continue;
        if (std::optional<lsSymbolInformation> info =
                GetSymbolInfo(db, sym, false)) {
          if ((sym.kind == SymbolKind::Type &&
               IgnoreType(db->GetType(sym).AnyDef())) ||
              (sym.kind == SymbolKind::Var &&
               IgnoreVar(db->GetVar(sym).AnyDef())))
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
