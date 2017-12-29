#include "clang_complete.h"
#include "message_handler.h"
#include "working_files.h"

namespace {
struct Ipc_TextDocumentDidChange
    : public IpcMessage<Ipc_TextDocumentDidChange> {
  const static IpcId kIpcId = IpcId::TextDocumentDidChange;
  lsTextDocumentDidChangeParams params;
};

MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidChange, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDidChange);

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
}  // namespace