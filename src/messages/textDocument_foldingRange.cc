// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.hh"
#include "pipeline.hh"
#include "project.hh"
#include "query_utils.h"
#include "working_files.h"

namespace ccls {
namespace {
struct FoldingRange {
  int startLine, startCharacter, endLine, endCharacter;
  std::string kind = "region";
};
MAKE_REFLECT_STRUCT(FoldingRange, startLine, startCharacter, endLine,
                    endCharacter, kind);
} // namespace

void MessageHandler::textDocument_foldingRange(TextDocumentParam &param,
                                               ReplyOnce &reply) {
  QueryFile *file = FindFile(reply, param.textDocument.uri.GetPath());
  if (!file)
    return;
  WorkingFile *wfile = working_files->GetFileByFilename(file->def->path);
  if (!wfile)
    return;
  std::vector<FoldingRange> result;
  std::optional<lsRange> ls_range;

  for (auto [sym, refcnt] : file->symbol2refcnt)
    if (refcnt > 0 && sym.extent.Valid() &&
        (sym.kind == SymbolKind::Func || sym.kind == SymbolKind::Type) &&
        (ls_range = GetLsRange(wfile, sym.extent))) {
      FoldingRange &fold = result.emplace_back();
      fold.startLine = ls_range->start.line;
      fold.startCharacter = ls_range->start.character;
      fold.endLine = ls_range->end.line;
      fold.endCharacter = ls_range->end.character;
    }
  reply(result);
}
} // namespace ccls
