// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "clang_complete.hh"
#include "message_handler.h"
#include "pipeline.hh"
#include "project.h"
using namespace ccls;

namespace {
MethodType kMethodType = "textDocument/didSave";

struct In_TextDocumentDidSave : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType; }

  struct Params {
    // The document that was saved.
    lsTextDocumentIdentifier textDocument;

    // Optional the content when saved. Depends on the includeText value
    // when the save notifcation was requested.
    // std::string text;
  } params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentDidSave::Params, textDocument);
MAKE_REFLECT_STRUCT(In_TextDocumentDidSave, params);
REGISTER_IN_MESSAGE(In_TextDocumentDidSave);

struct Handler_TextDocumentDidSave
    : BaseMessageHandler<In_TextDocumentDidSave> {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(In_TextDocumentDidSave *request) override {
    const auto &params = request->params;
    std::string path = params.textDocument.uri.GetPath();

    Project::Entry entry = project->FindCompilationEntryForFile(path);
    pipeline::Index(entry.filename, entry.args, IndexMode::Normal);
    clang_complete->NotifySave(path);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDidSave);
} // namespace
