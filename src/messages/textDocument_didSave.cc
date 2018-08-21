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
  };
  Params params;
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

    // Send out an index request, and copy the current buffer state so we
    // can update the cached index contents when the index is done.
    //
    // We also do not index if there is already an index request or if
    // the client requested indexing on didChange instead.
    //
    // TODO: Cancel outgoing index request. Might be tricky to make
    //       efficient since we have to cancel.
    //    - we could have an |atomic<int> active_cancellations| variable
    //      that all of the indexers check before accepting an index. if
    //      zero we don't slow down fast-path. if non-zero we acquire
    //      mutex and check to see if we should skip the current request.
    //      if so, ignore that index response.
    // TODO: send as priority request
    if (!g_config->index.onDidChange) {
      Project::Entry entry = project->FindCompilationEntryForFile(path);
      pipeline::Index(entry.filename, entry.args, true);
    }

    clang_complete->NotifySave(path);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDidSave);
} // namespace
