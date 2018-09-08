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
    const auto &params = request->params;
    std::string path = params.textDocument.uri.GetPath();
    working_files->OnChange(params);
    if (g_config->index.onChange) {
      Project::Entry entry = project->FindCompilationEntryForFile(path);
      pipeline::Index(entry.filename, entry.args, IndexMode::OnChange);
    }
    clang_complete->NotifyView(path);
    if (g_config->diagnostics.onChange)
      clang_complete->DiagnosticsUpdate(
          params.textDocument.AsTextDocumentIdentifier());
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDidChange);
} // namespace
