#include "message_handler.h"

struct TextDocumentDidCloseHandler
    : BaseMessageHandler<Ipc_TextDocumentDidClose> {
  void Run(Ipc_TextDocumentDidClose* request) override {
    std::string path = request->params.textDocument.uri.GetPath();

    // Clear any diagnostics for the file.
    Out_TextDocumentPublishDiagnostics out;
    out.params.uri = request->params.textDocument.uri;
    IpcManager::WriteStdout(IpcId::TextDocumentPublishDiagnostics, out);

    // Remove internal state.
    working_files->OnClose(request->params);
    clang_complete->NotifyClose(path);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentDidCloseHandler);
