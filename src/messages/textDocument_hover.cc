// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
using namespace ccls;

namespace {
MethodType kMethodType = "textDocument/hover";

const char *LanguageIdentifier(LanguageId lang) {
  switch (lang) {
  // clang-format off
  case LanguageId::C: return "c";
  case LanguageId::Cpp: return "cpp";
  case LanguageId::ObjC: return "objective-c";
  case LanguageId::ObjCpp: return "objective-cpp";
  default: return "";
  // clang-format on
  }
}

// Returns the hover or detailed name for `sym`, if any.
std::pair<std::optional<lsMarkedString>, std::optional<lsMarkedString>>
GetHover(DB *db, LanguageId lang, SymbolRef sym, int file_id) {
  const char *comments = nullptr;
  std::optional<lsMarkedString> ls_comments, hover;
  WithEntity(db, sym, [&](const auto &entity) {
    std::remove_reference_t<decltype(entity.def[0])> *def = nullptr;
    for (auto &d : entity.def) {
      if (d.spell) {
        comments = d.comments[0] ? d.comments : nullptr;
        def = &d;
        if (d.spell->file_id == file_id)
          break;
      }
    }
    if (!def && entity.def.size()) {
      def = &entity.def[0];
      if (def->comments[0])
        comments = def->comments;
    }
    if (def) {
      lsMarkedString m;
      m.language = LanguageIdentifier(lang);
      if (def->hover[0]) {
        m.value = def->hover;
        hover = m;
      } else if (def->detailed_name[0]) {
        m.value = def->detailed_name;
        hover = m;
      }
      if (comments)
        ls_comments = lsMarkedString{std::nullopt, comments};
    }
  });
  return {hover, ls_comments};
}

struct In_TextDocumentHover : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentHover, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentHover);

struct Out_TextDocumentHover : public lsOutMessage<Out_TextDocumentHover> {
  struct Result {
    std::vector<lsMarkedString> contents;
    std::optional<lsRange> range;
  };

  lsRequestId id;
  std::optional<Result> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentHover::Result, contents, range);
MAKE_REFLECT_STRUCT_MANDATORY_OPTIONAL(Out_TextDocumentHover, jsonrpc, id,
                                       result);

struct Handler_TextDocumentHover : BaseMessageHandler<In_TextDocumentHover> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_TextDocumentHover *request) override {
    auto &params = request->params;
    QueryFile *file;
    if (!FindFileOrFail(db, project, request->id,
                        params.textDocument.uri.GetPath(), &file))
      return;

    WorkingFile *working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_TextDocumentHover out;
    out.id = request->id;

    for (SymbolRef sym :
         FindSymbolsAtLocation(working_file, file, params.position)) {
      // Found symbol. Return hover.
      std::optional<lsRange> ls_range = GetLsRange(
          working_files->GetFileByFilename(file->def->path), sym.range);
      if (!ls_range)
        continue;

      auto[hover, comments] = GetHover(db, file->def->language, sym, file->id);
      if (comments || hover) {
        out.result = Out_TextDocumentHover::Result();
        out.result->range = *ls_range;
        if (comments)
          out.result->contents.push_back(*comments);
        if (hover)
          out.result->contents.push_back(*hover);
        break;
      }
    }

    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentHover);
} // namespace
