#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {

struct Ipc_TextDocumentTypeDefinition
    : public RequestMessage<Ipc_TextDocumentTypeDefinition> {
  const static IpcId kIpcId = IpcId::TextDocumentTypeDefinition;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentTypeDefinition, id, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentTypeDefinition);

struct Out_TextDocumentTypeDefinition
    : public lsOutMessage<Out_TextDocumentTypeDefinition> {
  lsRequestId id;
  std::vector<lsLocationEx> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentTypeDefinition, jsonrpc, id, result);

struct TextDocumentTypeDefinitionHandler
    : BaseMessageHandler<Ipc_TextDocumentTypeDefinition> {
  void Run(Ipc_TextDocumentTypeDefinition* request) override {
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentTypeDefinitionHandler);

}  // namespace
