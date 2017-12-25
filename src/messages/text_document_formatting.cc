#include "message_handler.h"

namespace {
struct Ipc_TextDocumentFormatting
    : public IpcMessage<Ipc_TextDocumentFormatting> {
  const static IpcId kIpcId = IpcId::TextDocumentFormatting;

  lsRequestId id;
  lsTextDocumentFormattingParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentFormatting, id, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentFormatting);

struct Out_TextDocumentFormatting
    : public lsOutMessage<Out_TextDocumentFormatting> {
  lsRequestId id;
  std::vector<lsTextEdit> edits;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentFormatting, jsonrpc, id, edits);
}  // namespace
