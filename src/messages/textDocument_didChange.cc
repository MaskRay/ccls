// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "clang_complete.h"
#include "message_handler.h"
#include "pipeline.hh"
#include "project.h"
#include "working_files.h"
using namespace ccls;

namespace {
MethodType kMethodType = "textDocument/didChange";

struct In_TextDocumentDidChange : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsTextDocumentDidChangeParams params;
};

MAKE_REFLECT_STRUCT(In_TextDocumentDidChange, params);
REGISTER_IN_MESSAGE(In_TextDocumentDidChange);

struct Handler_TextDocumentDidChange
    : BaseMessageHandler<In_TextDocumentDidChange> {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(In_TextDocumentDidChange *request) override {
    std::string path = request->params.textDocument.uri.GetPath();
    working_files->OnChange(request->params);
    if (g_config->index.onDidChange) {
      Project::Entry entry = project->FindCompilationEntryForFile(path);
      pipeline::Index(entry.filename, entry.args, true);
    }
    clang_complete->NotifyEdit(path);
    clang_complete->DiagnosticsUpdate(
        request->params.textDocument.AsTextDocumentIdentifier());
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDidChange);
} // namespace
