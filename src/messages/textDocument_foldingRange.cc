// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.hh"
#include "pipeline.hh"
#include "project.hh"
#include "query.hh"
#include "working_files.hh"

namespace ccls {
namespace {
struct FoldingRange {
  int startLine, startCharacter, endLine, endCharacter;
  std::string kind = "region";
};
REFLECT_STRUCT(FoldingRange, startLine, startCharacter, endLine, endCharacter,
               kind);
} // namespace

void MessageHandler::textDocument_foldingRange(TextDocumentParam &param,
                                               ReplyOnce &reply) {
  auto [file, wf] = findOrFail(param.textDocument.uri.getPath(), reply);
  if (!wf)
    return;
  std::vector<FoldingRange> result;
  std::optional<lsRange> ls_range;

  for (auto [sym, refcnt] : file->symbol2refcnt)
    if (refcnt > 0 && sym.extent.valid() &&
        (sym.kind == Kind::Func || sym.kind == Kind::Type) &&
        (ls_range = getLsRange(wf, sym.extent))) {
      FoldingRange &fold = result.emplace_back();
      fold.startLine = ls_range->start.line;
      fold.startCharacter = ls_range->start.character;
      fold.endLine = ls_range->end.line;
      fold.endCharacter = ls_range->end.character;
    }
  reply(result);
}
} // namespace ccls
