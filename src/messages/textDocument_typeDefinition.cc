// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
using namespace ccls;

namespace {
MethodType kMethodType = "textDocument/typeDefinition";

struct In_TextDocumentTypeDefinition : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentTypeDefinition, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentTypeDefinition);

struct Out_TextDocumentTypeDefinition
    : public lsOutMessage<Out_TextDocumentTypeDefinition> {
  lsRequestId id;
  std::vector<lsLocationEx> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentTypeDefinition, jsonrpc, id, result);

struct Handler_TextDocumentTypeDefinition
    : BaseMessageHandler<In_TextDocumentTypeDefinition> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_TextDocumentTypeDefinition *request) override {
    QueryFile *file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file,
                        nullptr))
      return;
    WorkingFile *working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_TextDocumentTypeDefinition out;
    out.id = request->id;
    auto Add = [&](const QueryType &type) {
      for (const auto &def : type.def)
        if (def.spell) {
          if (auto ls_loc = GetLsLocationEx(db, working_files, *def.spell,
                                            g_config->xref.container))
            out.result.push_back(*ls_loc);
        }
      if (out.result.empty())
        for (const DeclRef &dr : type.declarations)
          if (auto ls_loc = GetLsLocationEx(db, working_files, dr,
                                            g_config->xref.container))
            out.result.push_back(*ls_loc);
    };
    for (SymbolRef sym :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      switch (sym.kind) {
      case SymbolKind::Var: {
        const QueryVar::Def *def = db->GetVar(sym).AnyDef();
        if (def && def->type)
          Add(db->Type(def->type));
        break;
      }
      case SymbolKind::Type: {
        for (auto &def : db->GetType(sym).def)
          if (def.alias_of) {
            Add(db->Type(def.alias_of));
            break;
          }
        break;
      }
      default:
        break;
      }
    }

    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentTypeDefinition);

} // namespace
