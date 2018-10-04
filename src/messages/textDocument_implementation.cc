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
MethodType kMethodType = "textDocument/implementation";

struct In_TextDocumentImplementation : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentImplementation, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentImplementation);

struct Handler_TextDocumentImplementation
    : BaseMessageHandler<In_TextDocumentImplementation> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_TextDocumentImplementation *request) override {
    QueryFile *file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    WorkingFile *working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_LocationList out;
    out.id = request->id;
    for (SymbolRef sym :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      if (sym.kind == SymbolKind::Type) {
        QueryType &type = db->GetType(sym);
        out.result = GetLsLocations(db, working_files,
                                    GetTypeDeclarations(db, type.derived));
        break;
      } else if (sym.kind == SymbolKind::Func) {
        QueryFunc &func = db->GetFunc(sym);
        out.result = GetLsLocations(db, working_files,
                                    GetFuncDeclarations(db, func.derived));
        break;
      }
    }
    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentImplementation);
} // namespace
