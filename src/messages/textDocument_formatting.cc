// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.hh"
#include "pipeline.hh"
#include "working_files.hh"

#include <clang/Format/Format.h>
#include <clang/Tooling/Core/Replacement.h>

namespace ccls {
using namespace clang;

namespace {
llvm::Expected<tooling::Replacements> formatCode(StringRef code, StringRef file,
                                                 tooling::Range range) {
  auto style = format::getStyle("file", file, "LLVM", code, nullptr);
  if (!style)
    return style.takeError();
  tooling::Replacements includeReplaces =
      format::sortIncludes(*style, code, {range}, file);
  auto changed = tooling::applyAllReplacements(code, includeReplaces);
  if (!changed)
    return changed.takeError();
  return includeReplaces.merge(format::reformat(
      *style, *changed,
      tooling::calculateRangesAfterReplacements(includeReplaces, {range}),
      file));
}

std::vector<TextEdit> replacementsToEdits(std::string_view code,
                                          const tooling::Replacements &repls) {
  std::vector<TextEdit> ret;
  int i = 0, line = 0, col = 0;
  auto move = [&](int p) {
    for (; i < p; i++)
      if (code[i] == '\n')
        line++, col = 0;
      else {
        if ((uint8_t)code[i] >= 128) {
          while (128 <= (uint8_t)code[++i] && (uint8_t)code[i] < 192)
            ;
          i--;
        }
        col++;
      }
  };
  for (const auto &r : repls) {
    move(r.getOffset());
    int l = line, c = col;
    move(r.getOffset() + r.getLength());
    ret.push_back({{{l, c}, {line, col}}, r.getReplacementText().str()});
  }
  return ret;
}

void format(ReplyOnce &reply, WorkingFile *wfile, tooling::Range range) {
  std::string_view code = wfile->buffer_content;
  auto replsOrErr = formatCode(
      StringRef(code.data(), code.size()),
      StringRef(wfile->filename.data(), wfile->filename.size()), range);
  if (replsOrErr)
    reply(replacementsToEdits(code, *replsOrErr));
  else
    reply.error(ErrorCode::UnknownErrorCode,
                llvm::toString(replsOrErr.takeError()));
}
} // namespace

void MessageHandler::textDocument_formatting(DocumentFormattingParam &param,
                                             ReplyOnce &reply) {
  auto [file, wf] = findOrFail(param.textDocument.uri.getPath(), reply);
  if (!wf)
    return;
  format(reply, wf, {0, (unsigned)wf->buffer_content.size()});
}

void MessageHandler::textDocument_onTypeFormatting(
    DocumentOnTypeFormattingParam &param, ReplyOnce &reply) {
  auto [file, wf] = findOrFail(param.textDocument.uri.getPath(), reply);
  if (!wf) {
    return;
  }
  std::string_view code = wf->buffer_content;
  int pos = getOffsetForPosition(param.position, code);
  auto lbrace = code.find_last_of('{', pos);
  if (lbrace == std::string::npos)
    lbrace = pos;
  format(reply, wf, {(unsigned)lbrace, unsigned(pos - lbrace)});
}

void MessageHandler::textDocument_rangeFormatting(
    DocumentRangeFormattingParam &param, ReplyOnce &reply) {
  auto [file, wf] = findOrFail(param.textDocument.uri.getPath(), reply);
  if (!wf) {
    return;
  }
  std::string_view code = wf->buffer_content;
  int begin = getOffsetForPosition(param.range.start, code),
      end = getOffsetForPosition(param.range.end, code);
  format(reply, wf, {(unsigned)begin, unsigned(end - begin)});
}
} // namespace ccls
