// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.h"
#include "pipeline.hh"
#include "working_files.h"
using namespace ccls;

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

struct lsCodeAction {
  std::string title;
  const char *kind = "quickfix";
  lsWorkspaceEdit edit;
};
MAKE_REFLECT_STRUCT(lsCodeAction, title, kind, edit);

struct Out_TextDocumentCodeAction
    : public lsOutMessage<Out_TextDocumentCodeAction> {
  lsRequestId id;
  std::vector<lsCodeAction> result;
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
        lsCodeAction &cmd = out.result.emplace_back();
        cmd.title = "FixIt: " + diag.message;
        auto &edit = cmd.edit.documentChanges.emplace_back();
        edit.textDocument.uri = params.textDocument.uri;
        edit.textDocument.version = wfile->version;
        edit.edits = diag.fixits_;
      }
    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentCodeAction);
} // namespace
