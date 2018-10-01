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

#include "clang_complete.hh"
#include "include_complete.h"
#include "message_handler.h"
#include "pipeline.hh"
#include "project.h"
#include "working_files.h"
using namespace ccls;

namespace {
MethodType didChange = "textDocument/didChange";
MethodType didClose = "textDocument/didClose";
MethodType didOpen = "textDocument/didOpen";
MethodType didSave = "textDocument/didSave";

struct In_TextDocumentDidChange : public NotificationInMessage {
  MethodType GetMethodType() const override { return didChange; }
  lsTextDocumentDidChangeParams params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentDidChange, params);
REGISTER_IN_MESSAGE(In_TextDocumentDidChange);

struct Handler_TextDocumentDidChange
    : BaseMessageHandler<In_TextDocumentDidChange> {
  MethodType GetMethodType() const override { return didChange; }

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

struct In_TextDocumentDidClose : public NotificationInMessage {
  MethodType GetMethodType() const override { return didClose; }
  struct Params {
    lsTextDocumentIdentifier textDocument;
  } params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentDidClose::Params, textDocument);
MAKE_REFLECT_STRUCT(In_TextDocumentDidClose, params);
REGISTER_IN_MESSAGE(In_TextDocumentDidClose);

struct Handler_TextDocumentDidClose
    : BaseMessageHandler<In_TextDocumentDidClose> {
  MethodType GetMethodType() const override { return didClose; }

  void Run(In_TextDocumentDidClose *request) override {
    std::string path = request->params.textDocument.uri.GetPath();

    // Clear any diagnostics for the file.
    Out_TextDocumentPublishDiagnostics out;
    out.params.uri = request->params.textDocument.uri;
    pipeline::WriteStdout(didClose, out);

    // Remove internal state.
    working_files->OnClose(request->params.textDocument);
    clang_complete->OnClose(path);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDidClose);

struct In_TextDocumentDidOpen : public NotificationInMessage {
  MethodType GetMethodType() const override { return didOpen; }

  struct Params {
    lsTextDocumentItem textDocument;

    // ccls extension
    // If specified (e.g. ["clang++", "-DM", "a.cc"]), it overrides the project
    // entry (e.g. loaded from compile_commands.json or .ccls).
    std::vector<std::string> args;
  } params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentDidOpen::Params, textDocument, args);
MAKE_REFLECT_STRUCT(In_TextDocumentDidOpen, params);
REGISTER_IN_MESSAGE(In_TextDocumentDidOpen);

struct Handler_TextDocumentDidOpen
    : BaseMessageHandler<In_TextDocumentDidOpen> {
  MethodType GetMethodType() const override { return didOpen; }

  void Run(In_TextDocumentDidOpen *request) override {
    // NOTE: This function blocks code lens. If it starts taking a long time
    // we will need to find a way to unblock the code lens request.
    const auto &params = request->params;
    const std::string &path = params.textDocument.uri.GetPath();

    WorkingFile *working_file = working_files->OnOpen(params.textDocument);
    if (std::optional<std::string> cached_file_contents =
            pipeline::LoadIndexedContent(path))
      working_file->SetIndexContent(*cached_file_contents);

    QueryFile *file = nullptr;
    FindFileOrFail(db, project, std::nullopt, path, &file);
    if (file && file->def) {
      EmitSkippedRanges(working_file, file->def->skipped_ranges);
      EmitSemanticHighlighting(db, working_file, file);
    }

    include_complete->AddFile(working_file->filename);
    std::vector<const char *> args;
    for (const std::string &arg : params.args)
      args.push_back(Intern(arg));
    if (args.size())
      project->SetArgsForFile(args, path);

    // Submit new index request if it is not a header file or there is no
    // pending index request.
    if (SourceFileLanguage(path) != LanguageId::Unknown ||
        !pipeline::pending_index_requests)
      pipeline::Index(path, args, IndexMode::Normal);

    clang_complete->NotifyView(path);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDidOpen);

struct In_TextDocumentDidSave : public NotificationInMessage {
  MethodType GetMethodType() const override { return didSave; }

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
  MethodType GetMethodType() const override { return didSave; }

  void Run(In_TextDocumentDidSave *request) override {
    const auto &params = request->params;
    const std::string &path = params.textDocument.uri.GetPath();
    pipeline::Index(path, {}, IndexMode::Normal);
    clang_complete->NotifySave(path);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDidSave);
} // namespace
