// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.hh"
#include "pipeline.hh"
#include "query_utils.h"

namespace ccls {
namespace {
struct Param : TextDocumentPositionParam {
  // 1: field
  // 2: local
  // 4: parameter
  unsigned kind = ~0u;
};
MAKE_REFLECT_STRUCT(Param, textDocument, position, kind);
} // namespace

void MessageHandler::ccls_vars(Reader &reader, ReplyOnce &reply) {
  Param param;
  Reflect(reader, param);
  QueryFile *file = FindFile(reply, param.textDocument.uri.GetPath());
  if (!file)
    return;
  WorkingFile *working_file = working_files->GetFileByFilename(file->def->path);

  std::vector<lsLocation> result;
  for (SymbolRef sym :
       FindSymbolsAtLocation(working_file, file, param.position)) {
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
          GetVarDeclarations(db, db->Type(usr).instances, param.kind));
      break;
    }
  }
  reply(result);
}
} // namespace ccls
