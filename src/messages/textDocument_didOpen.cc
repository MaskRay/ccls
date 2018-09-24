// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "clang_complete.hh"
#include "include_complete.h"
#include "message_handler.h"
#include "pipeline.hh"
#include "project.h"
#include "working_files.h"
using namespace ccls;

namespace {
MethodType kMethodType = "textDocument/didOpen";

struct In_TextDocumentDidOpen : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType; }

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
  MethodType GetMethodType() const override { return kMethodType; }

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

    // Submit new index request if it is not a header file.
    if (SourceFileLanguage(path) != LanguageId::Unknown)
      pipeline::Index(path, args, IndexMode::Normal);

    clang_complete->NotifyView(path);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDidOpen);
} // namespace
