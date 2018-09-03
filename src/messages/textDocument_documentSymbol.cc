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

namespace {
MethodType kMethodType = "textDocument/documentSymbol";

struct lsDocumentSymbolParams {
  lsTextDocumentIdentifier textDocument;
  int startLine = -1;
  int endLine = -1;
};
MAKE_REFLECT_STRUCT(lsDocumentSymbolParams, textDocument, startLine, endLine);

struct In_TextDocumentDocumentSymbol : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsDocumentSymbolParams params;
};
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

    if (params.startLine >= 0) {
      Out_SimpleDocumentSymbol out;
      out.id = request->id;
      for (auto [sym, refcnt] : file->symbol2refcnt)
        if (refcnt > 0 && params.startLine <= sym.range.start.line &&
            sym.range.start.line <= params.endLine)
          if (auto ls_loc = GetLsLocation(
                  db, working_files,
                  Use{{sym.range, sym.usr, sym.kind, sym.role}, file_id}))
            out.result.push_back(ls_loc->range);
      std::sort(out.result.begin(), out.result.end());
      pipeline::WriteStdout(kMethodType, out);
    } else {
      Out_TextDocumentDocumentSymbol out;
      out.id = request->id;
      for (auto [sym, refcnt] : file->outline2refcnt) {
        if (refcnt <= 0) continue;
        if (std::optional<lsSymbolInformation> info =
                GetSymbolInfo(db, working_files, sym, false)) {
          if (sym.kind == SymbolKind::Var) {
            QueryVar &var = db->GetVar(sym);
            auto *def = var.AnyDef();
            if (!def || !def->spell || def->is_local())
              continue;
          }
          if (std::optional<lsLocation> location = GetLsLocation(
                  db, working_files,
                  Use{{sym.range, sym.usr, sym.kind, sym.role}, file_id})) {
            info->location = *location;
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
