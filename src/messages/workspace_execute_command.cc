#include "lsp_code_action.h"
#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
MethodType kMethodType = "workspace/executeCommand";

struct In_WorkspaceExecuteCommand : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsCommand<lsCodeLensCommandArguments> params;
};
MAKE_REFLECT_STRUCT(In_WorkspaceExecuteCommand, id, params);
REGISTER_IN_MESSAGE(In_WorkspaceExecuteCommand);

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

struct Handler_WorkspaceExecuteCommand
    : BaseMessageHandler<In_WorkspaceExecuteCommand> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_WorkspaceExecuteCommand* request) override {
    const auto& params = request->params;
    Out_WorkspaceExecuteCommand out;
    out.id = request->id;
    if (params.command == "cquery._applyFixIt") {
    } else if (params.command == "cquery._autoImplement") {
    } else if (params.command == "cquery._insertInclude") {
    } else if (params.command == "cquery.showReferences") {
      out.result = params.arguments.locations;
    }

    QueueManager::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_WorkspaceExecuteCommand);

}  // namespace
