#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
using namespace ccls;

namespace {
MethodType kMethodType = "textDocument/hover";

// Find the comments for |sym|, if any.
std::optional<lsMarkedString> GetComments(DB* db, SymbolRef sym) {
  std::optional<lsMarkedString> ret;
  WithEntity(db, sym, [&](const auto& entity) {
    if (const auto* def = entity.AnyDef())
      if (!def->comments.empty()) {
        lsMarkedString m;
        m.value = def->comments;
        ret = m;
      }
  });
  return ret;
}

// Returns the hover or detailed name for `sym`, if any.
std::optional<lsMarkedString> GetHoverOrName(DB* db,
                                             LanguageId lang,
                                             SymbolRef sym) {
  std::optional<lsMarkedString> ret;
  WithEntity(db, sym, [&](const auto& entity) {
    if (const auto* def = entity.AnyDef()) {
      lsMarkedString m;
      m.language = LanguageIdentifier(lang);
      if (!def->hover.empty()) {
        m.value = def->hover;
        ret = m;
      } else if (!def->detailed_name.empty()) {
        m.value = def->detailed_name;
        ret = m;
      }
    }
  });
  return ret;
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
MAKE_REFLECT_STRUCT_MANDATORY_OPTIONAL(Out_TextDocumentHover,
                                       jsonrpc,
                                       id,
                                       result);

struct Handler_TextDocumentHover : BaseMessageHandler<In_TextDocumentHover> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_TextDocumentHover* request) override {
    auto& params = request->params;
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        params.textDocument.uri.GetPath(), &file))
      return;

    WorkingFile* working_file =
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

      std::optional<lsMarkedString> comments = GetComments(db, sym);
      std::optional<lsMarkedString> hover =
          GetHoverOrName(db, file->def->language, sym);
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
}  // namespace
