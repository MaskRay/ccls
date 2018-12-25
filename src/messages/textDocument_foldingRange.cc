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
  auto [file, wf] = FindOrFail(param.textDocument.uri.GetPath(), reply);
  if (!wf)
    return;
  std::vector<FoldingRange> result;
  std::optional<lsRange> ls_range;

  for (auto [sym, refcnt] : file->symbol2refcnt)
    if (refcnt > 0 && sym.extent.Valid() &&
        (sym.kind == Kind::Func || sym.kind == Kind::Type) &&
        (ls_range = GetLsRange(wf, sym.extent))) {
      FoldingRange &fold = result.emplace_back();
      fold.startLine = ls_range->start.line;
      fold.startCharacter = ls_range->start.character;
      fold.endLine = ls_range->end.line;
      fold.endCharacter = ls_range->end.character;
    }
  reply(result);
}
} // namespace ccls
