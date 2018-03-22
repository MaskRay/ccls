#include "clang_format.h"
#include "message_handler.h"
#include "queue_manager.h"
#include "working_files.h"

#include <loguru.hpp>

namespace {
MethodType kMethodType = "textDocument/formatting";

struct In_TextDocumentFormatting : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct Params {
    lsTextDocumentIdentifier textDocument;
    lsFormattingOptions options;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentFormatting::Params, textDocument, options);
MAKE_REFLECT_STRUCT(In_TextDocumentFormatting, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentFormatting);

struct Out_TextDocumentFormatting
    : public lsOutMessage<Out_TextDocumentFormatting> {
  lsRequestId id;
  std::vector<lsTextEdit> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentFormatting, jsonrpc, id, result);

struct Handler_TextDocumentFormatting
    : BaseMessageHandler<In_TextDocumentFormatting> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_TextDocumentFormatting* request) override {
    Out_TextDocumentFormatting response;
    response.id = request->id;
#if USE_CLANG_CXX
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    response.result = ConvertClangReplacementsIntoTextEdits(
        working_file->buffer_content,
        ClangFormatDocument(working_file, 0,
                            working_file->buffer_content.size(),
                            request->params.options));
#else
    LOG_S(WARNING) << "You must compile cquery with --use-clang-cxx to use "
                      "textDocument/formatting.";
    // TODO: Fallback to execute the clang-format binary?
    response.result = {};
#endif

    QueueManager::WriteStdout(kMethodType, response);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentFormatting);
}  // namespace
