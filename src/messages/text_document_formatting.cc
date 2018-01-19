#include "clang_format.h"
#include "message_handler.h"
#include "queue_manager.h"
#include "working_files.h"

#include <loguru.hpp>

namespace {

struct Ipc_TextDocumentFormatting
    : public RequestMessage<Ipc_TextDocumentFormatting> {
  const static IpcId kIpcId = IpcId::TextDocumentFormatting;
  struct Params {
    lsTextDocumentIdentifier textDocument;
    lsFormattingOptions options;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentFormatting::Params, textDocument, options);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentFormatting, id, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentFormatting);

struct Out_TextDocumentFormatting
    : public lsOutMessage<Out_TextDocumentFormatting> {
  lsRequestId id;
  std::vector<lsTextEdit> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentFormatting, jsonrpc, id, result);

struct TextDocumentFormattingHandler
    : BaseMessageHandler<Ipc_TextDocumentFormatting> {
  void Run(Ipc_TextDocumentFormatting* request) override {
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

    QueueManager::WriteStdout(IpcId::TextDocumentFormatting, response);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentFormattingHandler);
}  // namespace
