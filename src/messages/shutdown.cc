#include "message_handler.h"
#include "queue_manager.h"

namespace {
struct Ipc_Shutdown : public IpcMessage<Ipc_Shutdown> {
  static const IpcId kIpcId = IpcId::Shutdown;
};
MAKE_REFLECT_EMPTY_STRUCT(Ipc_Shutdown);
REGISTER_IPC_MESSAGE(Ipc_Shutdown);

struct Out_Shutdown : public lsOutMessage<Out_Shutdown> {
  lsRequestId id;         // defaults to std::monostate (null)
  std::monostate result;  // null
};
MAKE_REFLECT_STRUCT(Out_Shutdown, jsonrpc, id, result);

struct ShutdownHandler : BaseMessageHandler<Ipc_Shutdown> {
  void Run(Ipc_Shutdown* request) override {
    Out_Shutdown out;
    QueueManager::WriteStdout(IpcId::TextDocumentDefinition, out);
  }
};
REGISTER_MESSAGE_HANDLER(ShutdownHandler);
}  // namespace
