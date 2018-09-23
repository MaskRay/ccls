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
#include "message_handler.h"
#include "pipeline.hh"
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
    const std::string &path = params.textDocument.uri.GetPath();
    pipeline::Index(path, {}, IndexMode::Normal);
    clang_complete->NotifySave(path);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDidSave);
} // namespace
