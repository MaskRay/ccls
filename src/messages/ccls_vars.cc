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
  Reflect(reader, param);
  QueryFile *file = FindFile(param.textDocument.uri.GetPath());
  WorkingFile *wf = file ? wfiles->GetFile(file->def->path) : nullptr;
  if (!wf) {
    reply.NotReady(file);
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
