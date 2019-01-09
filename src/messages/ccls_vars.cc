// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
using namespace ccls;

namespace {
MethodType kMethodType = "$ccls/vars";

struct In_cclsVars : public RequestMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct Params : lsTextDocumentPositionParams {
    // 1: field
    // 2: local
    // 4: parameter
    unsigned kind = ~0u;
  } params;
};
MAKE_REFLECT_STRUCT(In_cclsVars::Params, textDocument, position, kind);
MAKE_REFLECT_STRUCT(In_cclsVars, id, params);
REGISTER_IN_MESSAGE(In_cclsVars);

struct Handler_cclsVars : BaseMessageHandler<In_cclsVars> {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(In_cclsVars *request) override {
    auto &params = request->params;
    QueryFile *file;
    if (!FindFileOrFail(db, project, request->id,
                        params.textDocument.uri.GetPath(), &file))
      return;

    WorkingFile *working_file =
        working_files->GetFileByFilename(file->def->path);

    std::vector<lsLocation> result;
    for (SymbolRef sym :
         FindSymbolsAtLocation(working_file, file, params.position)) {
      Usr usr = sym.usr;
      switch (sym.kind) {
      default:
        break;
      case SymbolKind::Var: {
        const QueryVar::Def *def = db->GetVar(sym).AnyDef();
        if (!def || !def->type)
          continue;
        usr = def->type;
        [[fallthrough]];
      }
      case SymbolKind::Type:
        result = GetLsLocations(
            db, working_files,
            GetVarDeclarations(db, db->Type(usr).instances, params.kind));
        break;
      }
    }
    pipeline::Reply(request->id, result);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_cclsVars);
} // namespace
