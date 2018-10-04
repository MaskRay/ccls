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
MethodType kMethodType = "textDocument/typeDefinition";

struct In_TextDocumentTypeDefinition : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentTypeDefinition, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentTypeDefinition);

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

    Out_LocationList out;
    out.id = request->id;
    auto Add = [&](const QueryType &type) {
      for (const auto &def : type.def)
        if (def.spell) {
          if (auto ls_loc = GetLsLocation(db, working_files, *def.spell))
            out.result.push_back(*ls_loc);
        }
      if (out.result.empty())
        for (const DeclRef &dr : type.declarations)
          if (auto ls_loc = GetLsLocation(db, working_files, dr))
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
