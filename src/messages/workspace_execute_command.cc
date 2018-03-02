#include "lsp_code_action.h"
#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {

struct Ipc_WorkspaceExecuteCommand
    : public RequestMessage<Ipc_WorkspaceExecuteCommand> {
  const static IpcId kIpcId = IpcId::WorkspaceExecuteCommand;
  lsCommand<lsCodeLensCommandArguments> params;
};
MAKE_REFLECT_STRUCT(Ipc_WorkspaceExecuteCommand, id, params);
REGISTER_IPC_MESSAGE(Ipc_WorkspaceExecuteCommand);

struct Out_WorkspaceExecuteCommand
    : public lsOutMessage<Out_WorkspaceExecuteCommand> {
  lsRequestId id;
  std::variant<std::vector<lsLocation>, CommandArgs> result;
};
MAKE_REFLECT_STRUCT(Out_WorkspaceExecuteCommand, jsonrpc, id, result);

void Reflect(Writer& visitor, Out_WorkspaceExecuteCommand& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(jsonrpc);
  REFLECT_MEMBER(id);
  REFLECT_MEMBER(result);
  REFLECT_MEMBER_END();
}

struct WorkspaceExecuteCommandHandler
    : BaseMessageHandler<Ipc_WorkspaceExecuteCommand> {
  void Run(Ipc_WorkspaceExecuteCommand* request) override {
    const auto& params = request->params;
    Out_WorkspaceExecuteCommand out;
    out.id = request->id;
    if (params.command == "cquery._applyFixIt") {
    } else if (params.command == "cquery._autoImplement") {
    } else if (params.command == "cquery._insertInclude") {
    } else if (params.command == "cquery.showReferences") {
      out.result = params.arguments.locations;
    }

    QueueManager::WriteStdout(IpcId::WorkspaceExecuteCommand, out);
  }
};
REGISTER_MESSAGE_HANDLER(WorkspaceExecuteCommandHandler);

}  // namespace
