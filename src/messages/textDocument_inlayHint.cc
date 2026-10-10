// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.hh"
#include "query.hh"
#include "working_files.hh"

namespace ccls {
namespace {
struct InlayHint {
  Position position;
  std::string label;
  std::optional<int> kind;
  bool paddingLeft;
  bool paddingRight;
};
REFLECT_STRUCT(InlayHint, position, label, kind, paddingLeft, paddingRight);

bool isEnabled(InlayHintKind kind) {
  switch (kind) {
  case InlayHintKind::Type:
    return g_config->inlayHint.deducedTypes;
  case InlayHintKind::Parameter:
    return g_config->inlayHint.parameterNames;
  case InlayHintKind::Designator:
    return g_config->inlayHint.designators;
  case InlayHintKind::BlockEnd:
    return g_config->inlayHint.blockEnd;
  }
  return false;
}
} // namespace

void MessageHandler::textDocument_inlayHint(InlayHintParam &param, ReplyOnce &reply) {
  auto [file, wf] = findOrFail(param.textDocument.uri.getPath(), reply);
  if (!wf)
    return;
  std::vector<InlayHint> result;
  if (wf->index_lines.empty()) {
    reply(result);
    return;
  }
  for (const IndexInlayHint &hint : file->def->inlay_hints) {
    if (!isEnabled(hint.kind))
      continue;
    if (hint.pos.line >= wf->index_lines.size())
      continue;
    // Drop hints on lines edited since indexing, where they may be misplaced.
    int column = hint.pos.column;
    std::optional<int> line = wf->getBufferPosFromIndexPos(hint.pos.line, &column, false);
    if (!line || *line >= (int)wf->buffer_lines.size() || wf->buffer_lines[*line] != wf->index_lines[hint.pos.line])
      continue;
    Position pos{*line, column};
    if (pos < param.range.start || !(pos < param.range.end))
      continue;
    std::optional<int> kind;
    if (hint.kind <= InlayHintKind::Parameter)
      kind = int(hint.kind);
    result.push_back(
        {pos, hint.label, kind, hint.kind == InlayHintKind::BlockEnd, hint.kind == InlayHintKind::Parameter});
  }
  reply(result);
}
} // namespace ccls
