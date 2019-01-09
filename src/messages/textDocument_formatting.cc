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

#include "message_handler.h"
#include "pipeline.hh"
#include "working_files.h"

#include <clang/Format/Format.h>
#include <clang/Tooling/Core/Replacement.h>

using namespace ccls;
using namespace clang;

namespace {
const MethodType formatting = "textDocument/formatting",
                 onTypeFormatting = "textDocument/onTypeFormatting",
                 rangeFormatting = "textDocument/rangeFormatting";

struct lsFormattingOptions {
  // Size of a tab in spaces.
  int tabSize;
  // Prefer spaces over tabs.
  bool insertSpaces;
};
MAKE_REFLECT_STRUCT(lsFormattingOptions, tabSize, insertSpaces);

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

std::vector<lsTextEdit>
ReplacementsToEdits(std::string_view code, const tooling::Replacements &Repls) {
  std::vector<lsTextEdit> ret;
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

void Format(WorkingFile *wfile, tooling::Range range, lsRequestId id) {
  std::string_view code = wfile->buffer_content;
  auto ReplsOrErr =
      FormatCode(code, wfile->filename, range);
  if (ReplsOrErr) {
    auto result = ReplacementsToEdits(code, *ReplsOrErr);
    pipeline::Reply(id, result);
  } else {
    lsResponseError err;
    err.code = lsErrorCodes::UnknownErrorCode;
    err.message = llvm::toString(ReplsOrErr.takeError());
    pipeline::ReplyError(id, err);
  }
}

struct In_TextDocumentFormatting : public RequestMessage {
  MethodType GetMethodType() const override { return formatting; }
  struct Params {
    lsTextDocumentIdentifier textDocument;
    lsFormattingOptions options;
  } params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentFormatting::Params, textDocument, options);
MAKE_REFLECT_STRUCT(In_TextDocumentFormatting, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentFormatting);

struct Handler_TextDocumentFormatting
    : BaseMessageHandler<In_TextDocumentFormatting> {
  MethodType GetMethodType() const override { return formatting; }
  void Run(In_TextDocumentFormatting *request) override {
    auto &params = request->params;
    QueryFile *file;
    if (!FindFileOrFail(db, project, request->id,
                        params.textDocument.uri.GetPath(), &file))
      return;
    WorkingFile *wfile = working_files->GetFileByFilename(file->def->path);
    if (!wfile)
      return;
    Format(wfile, {0, (unsigned)wfile->buffer_content.size()}, request->id);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentFormatting);

struct In_TextDocumentOnTypeFormatting : public RequestMessage {
  MethodType GetMethodType() const override { return onTypeFormatting; }
  struct Params {
    lsTextDocumentIdentifier textDocument;
    lsPosition position;
    std::string ch;
    lsFormattingOptions options;
  } params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentOnTypeFormatting::Params, textDocument,
                    position, ch, options);
MAKE_REFLECT_STRUCT(In_TextDocumentOnTypeFormatting, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentOnTypeFormatting);

struct Handler_TextDocumentOnTypeFormatting
    : BaseMessageHandler<In_TextDocumentOnTypeFormatting> {
  MethodType GetMethodType() const override { return onTypeFormatting; }
  void Run(In_TextDocumentOnTypeFormatting *request) override {
    auto &params = request->params;
    QueryFile *file;
    if (!FindFileOrFail(db, project, request->id,
                        params.textDocument.uri.GetPath(), &file))
      return;
    WorkingFile *wfile = working_files->GetFileByFilename(file->def->path);
    if (!wfile)
      return;
    std::string_view code = wfile->buffer_content;
    int pos = GetOffsetForPosition(params.position, code);
    auto lbrace = code.find_last_of('{', pos);
    if (lbrace == std::string::npos)
      lbrace = pos;
    Format(wfile, {(unsigned)lbrace, unsigned(pos - lbrace)}, request->id);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentOnTypeFormatting);

struct In_TextDocumentRangeFormatting : public RequestMessage {
  MethodType GetMethodType() const override { return rangeFormatting; }
  struct Params {
    lsTextDocumentIdentifier textDocument;
    lsRange range;
    lsFormattingOptions options;
  } params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentRangeFormatting::Params, textDocument, range,
                    options);
MAKE_REFLECT_STRUCT(In_TextDocumentRangeFormatting, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentRangeFormatting);

struct Handler_TextDocumentRangeFormatting
    : BaseMessageHandler<In_TextDocumentRangeFormatting> {
  MethodType GetMethodType() const override { return rangeFormatting; }

  void Run(In_TextDocumentRangeFormatting *request) override {
    auto &params = request->params;
    QueryFile *file;
    if (!FindFileOrFail(db, project, request->id,
                        params.textDocument.uri.GetPath(), &file))
      return;
    WorkingFile *wfile = working_files->GetFileByFilename(file->def->path);
    if (!wfile)
      return;
    std::string_view code = wfile->buffer_content;
    int begin = GetOffsetForPosition(params.range.start, code),
        end = GetOffsetForPosition(params.range.end, code);
    Format(wfile, {(unsigned)begin, unsigned(end - begin)}, request->id);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentRangeFormatting);
} // namespace
