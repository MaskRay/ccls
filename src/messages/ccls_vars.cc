// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

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
  reflect(reader, param);
  auto [file, wf] = findOrFail(param.textDocument.uri.getPath(), reply);
  if (!wf) {
    return;
  }

  std::vector<Location> result;
  for (SymbolRef sym : findSymbolsAtLocation(wf, file, param.position)) {
    Usr usr = sym.usr;
    switch (sym.kind) {
    default:
      break;
    case Kind::Var: {
      const QueryVar::Def *def = db->getVar(sym).anyDef();
      if (!def || !def->type)
        continue;
      usr = def->type;
      [[fallthrough]];
    }
    case Kind::Type: {
      for (DeclRef dr :
           getVarDeclarations(db, db->getType(usr).instances, param.kind))
        if (auto loc = getLocationLink(db, wfiles, dr))
          result.push_back(Location(std::move(loc)));
      break;
    }
    }
  }
  reply(result);
}
} // namespace ccls
