/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "lsp_code_action.h"
#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
using namespace ccls;

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

struct Handler_WorkspaceExecuteCommand
    : BaseMessageHandler<In_WorkspaceExecuteCommand> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_WorkspaceExecuteCommand *request) override {
    const auto &params = request->params;
    Out_WorkspaceExecuteCommand out;
    out.id = request->id;
    if (params.command == "ccls._applyFixIt") {
    } else if (params.command == "ccls._autoImplement") {
    } else if (params.command == "ccls._insertInclude") {
    } else if (params.command == "ccls.showReferences") {
      out.result = params.arguments.locations;
    }

    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_WorkspaceExecuteCommand);

} // namespace
