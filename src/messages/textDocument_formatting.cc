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
llvm::Expected<tooling::Replacements>
FormatCode(std::string_view code, std::string_view file, tooling::Range Range) {
  StringRef Code(code.data(), code.size()), File(file.data(), file.size());
  auto Style = format::getStyle("file", File, "LLVM", Code, nullptr);
  if (!Style)
    return Style.takeError();
  tooling::Replacements IncludeReplaces =
      format::sortIncludes(*Style, Code, {Range}, File);
  auto Changed = tooling::applyAllReplacements(Code, IncludeReplaces);
  if (!Changed)
    return Changed.takeError();
  return IncludeReplaces.merge(format::reformat(
      *Style, *Changed,
      tooling::calculateRangesAfterReplacements(IncludeReplaces, {Range}),
      File));
}

std::vector<TextEdit> ReplacementsToEdits(std::string_view code,
                                          const tooling::Replacements &Repls) {
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
  for (const auto &R : Repls) {
    move(R.getOffset());
    int l = line, c = col;
    move(R.getOffset() + R.getLength());
    ret.push_back({{{l, c}, {line, col}}, R.getReplacementText().str()});
  }
  return ret;
}

void Format(ReplyOnce &reply, WorkingFile *wfile, tooling::Range range) {
  std::string_view code = wfile->buffer_content;
  auto ReplsOrErr = FormatCode(code, wfile->filename, range);
  if (ReplsOrErr)
    reply(ReplacementsToEdits(code, *ReplsOrErr));
  else
    reply.Error(ErrorCode::UnknownErrorCode,
                llvm::toString(ReplsOrErr.takeError()));
}
} // namespace

void MessageHandler::textDocument_formatting(DocumentFormattingParam &param,
                                             ReplyOnce &reply) {
  QueryFile *file = FindFile(reply, param.textDocument.uri.GetPath());
  if (!file)
    return;
  WorkingFile *wfile = wfiles->GetFileByFilename(file->def->path);
  if (!wfile)
    return;
  Format(reply, wfile, {0, (unsigned)wfile->buffer_content.size()});
}

void MessageHandler::textDocument_onTypeFormatting(
    DocumentOnTypeFormattingParam &param, ReplyOnce &reply) {
  QueryFile *file = FindFile(reply, param.textDocument.uri.GetPath());
  if (!file)
    return;
  WorkingFile *wfile = wfiles->GetFileByFilename(file->def->path);
  if (!wfile)
    return;
  std::string_view code = wfile->buffer_content;
  int pos = GetOffsetForPosition(param.position, code);
  auto lbrace = code.find_last_of('{', pos);
  if (lbrace == std::string::npos)
    lbrace = pos;
  Format(reply, wfile, {(unsigned)lbrace, unsigned(pos - lbrace)});
}

void MessageHandler::textDocument_rangeFormatting(
    DocumentRangeFormattingParam &param, ReplyOnce &reply) {
  QueryFile *file = FindFile(reply, param.textDocument.uri.GetPath());
  if (!file)
    return;
  WorkingFile *wfile = wfiles->GetFileByFilename(file->def->path);
  if (!wfile)
    return;
  std::string_view code = wfile->buffer_content;
  int begin = GetOffsetForPosition(param.range.start, code),
      end = GetOffsetForPosition(param.range.end, code);
  Format(reply, wfile, {(unsigned)begin, unsigned(end - begin)});
}
} // namespace ccls
