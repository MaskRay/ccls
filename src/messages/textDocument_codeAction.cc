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

#include "message_handler.h"
#include "pipeline.hh"
#include "working_files.h"
using namespace ccls;

struct CommandArgs {
  lsDocumentUri textDocumentUri;
  std::vector<lsTextEdit> edits;
};
MAKE_REFLECT_STRUCT_WRITER_AS_ARRAY(CommandArgs, textDocumentUri, edits);

namespace {
MethodType kMethodType = "textDocument/codeAction";

struct In_TextDocumentCodeAction : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }

  // Contains additional diagnostic information about the context in which
  // a code action is run.
  struct lsCodeActionContext {
    // An array of diagnostics.
    std::vector<lsDiagnostic> diagnostics;
  };
  // Params for the CodeActionRequest
  struct lsCodeActionParams {
    // The document in which the command was invoked.
    lsTextDocumentIdentifier textDocument;
    // The range for which the command was invoked.
    lsRange range;
    // Context carrying additional information.
    lsCodeActionContext context;
  } params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentCodeAction::lsCodeActionContext,
                    diagnostics);
MAKE_REFLECT_STRUCT(In_TextDocumentCodeAction::lsCodeActionParams, textDocument,
                    range, context);
MAKE_REFLECT_STRUCT(In_TextDocumentCodeAction, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentCodeAction);

struct Out_TextDocumentCodeAction
    : public lsOutMessage<Out_TextDocumentCodeAction> {
  lsRequestId id;
  std::vector<lsCommand<CommandArgs>> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentCodeAction, jsonrpc, id, result);

struct Handler_TextDocumentCodeAction
    : BaseMessageHandler<In_TextDocumentCodeAction> {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(In_TextDocumentCodeAction *request) override {
    const auto &params = request->params;
    WorkingFile *wfile =
        working_files->GetFileByFilename(params.textDocument.uri.GetPath());
    if (!wfile)
      return;
    Out_TextDocumentCodeAction out;
    out.id = request->id;
    std::vector<lsDiagnostic> diagnostics;
    working_files->DoAction([&]() { diagnostics = wfile->diagnostics_; });
    for (lsDiagnostic &diag : diagnostics)
      if (diag.fixits_.size()) {
        lsCommand<CommandArgs> command;
        command.title = "FixIt: " + diag.message;
        command.command = "ccls._applyFixIt";
        command.arguments.textDocumentUri = params.textDocument.uri;
        command.arguments.edits = diag.fixits_;
        out.result.push_back(command);
      }
    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentCodeAction);
} // namespace
