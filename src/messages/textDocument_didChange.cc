// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "clang_complete.hh"
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
    const auto &params = request->params;
    std::string path = params.textDocument.uri.GetPath();
    working_files->OnChange(params);
    if (g_config->index.onChange)
      pipeline::Index(path, {}, IndexMode::OnChange);
    clang_complete->NotifyView(path);
    if (g_config->diagnostics.onChange >= 0)
      clang_complete->DiagnosticsUpdate(path, g_config->diagnostics.onChange);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDidChange);
} // namespace
