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

#include "message_handler.hh"
#include "pipeline.hh"
#include "query.hh"

namespace ccls {
namespace {
struct Param : TextDocumentPositionParam {
  // 1: field
  // 2: local
  // 4: parameter
  unsigned kind = ~0u;
};
REFLECT_STRUCT(Param, textDocument, position, kind);
} // namespace

void MessageHandler::ccls_vars(JsonReader &reader, ReplyOnce &reply) {
  Param param;
  Reflect(reader, param);
  auto [file, wf] = FindOrFail(param.textDocument.uri.GetPath(), reply);
  if (!wf) {
    return;
  }

  std::vector<Location> result;
  for (SymbolRef sym : FindSymbolsAtLocation(wf, file, param.position)) {
    Usr usr = sym.usr;
    switch (sym.kind) {
    default:
      break;
    case Kind::Var: {
      const QueryVar::Def *def = db->GetVar(sym).AnyDef();
      if (!def || !def->type)
        continue;
      usr = def->type;
      [[fallthrough]];
    }
    case Kind::Type: {
      for (DeclRef dr :
           GetVarDeclarations(db, db->Type(usr).instances, param.kind))
        if (auto loc = GetLocationLink(db, wfiles, dr))
          result.push_back(Location(std::move(loc)));
      break;
    }
    }
  }
  reply(result);
}
} // namespace ccls
