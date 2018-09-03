// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
using namespace ccls;
using namespace clang;

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

    const auto &symbol2refcnt =
        params.all ? file->symbol2refcnt : file->outline2refcnt;
    if (params.startLine >= 0) {
      Out_SimpleDocumentSymbol out;
      out.id = request->id;
      for (auto [sym, refcnt] : symbol2refcnt)
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
      for (auto [sym, refcnt] : symbol2refcnt) {
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
