#include "message_handler.h"

struct TextDocumentDidChangeHandler
    : BaseMessageHandler<Ipc_TextDocumentDidChange> {
  void Run(Ipc_TextDocumentDidChange* request) override {
    std::string path = request->params.textDocument.uri.GetPath();
    working_files->OnChange(request->params);
    clang_complete->NotifyEdit(path);
    clang_complete->DiagnosticsUpdate(
        request->params.textDocument.AsTextDocumentIdentifier());
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentDidChangeHandler);
