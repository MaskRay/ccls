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

namespace {
MethodType kMethodType = "$ccls/vars";

struct In_CclsVars : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct Params : lsTextDocumentPositionParams {
    // 1: field
    // 2: local
    // 4: parameter
    unsigned kind = ~0u;
  } params;
};
MAKE_REFLECT_STRUCT(In_CclsVars::Params, textDocument, position, kind);
MAKE_REFLECT_STRUCT(In_CclsVars, id, params);
REGISTER_IN_MESSAGE(In_CclsVars);

struct Handler_CclsVars : BaseMessageHandler<In_CclsVars> {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(In_CclsVars *request) override {
    auto &params = request->params;
    QueryFile *file;
    if (!FindFileOrFail(db, project, request->id,
                        params.textDocument.uri.GetPath(), &file))
      return;

    WorkingFile *working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_LocationList out;
    out.id = request->id;
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
        out.result = GetLsLocations(
            db, working_files,
            GetVarDeclarations(db, db->Type(usr).instances, params.kind));
        break;
      }
    }
    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CclsVars);
} // namespace
