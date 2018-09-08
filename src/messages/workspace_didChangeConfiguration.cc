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
#include "project.h"
#include "working_files.h"
using namespace ccls;

namespace {
MethodType kMethodType = "workspace/didChangeConfiguration";

struct lsDidChangeConfigurationParams {
  bool placeholder;
};
MAKE_REFLECT_STRUCT(lsDidChangeConfigurationParams, placeholder);

struct In_WorkspaceDidChangeConfiguration : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsDidChangeConfigurationParams params;
};
MAKE_REFLECT_STRUCT(In_WorkspaceDidChangeConfiguration, params);
REGISTER_IN_MESSAGE(In_WorkspaceDidChangeConfiguration);

struct Handler_WorkspaceDidChangeConfiguration
    : BaseMessageHandler<In_WorkspaceDidChangeConfiguration> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_WorkspaceDidChangeConfiguration *request) override {
    project->Load(g_config->projectRoot);
    project->Index(working_files, lsRequestId());

    clang_complete->FlushAllSessions();
  }
};
REGISTER_MESSAGE_HANDLER(Handler_WorkspaceDidChangeConfiguration);
} // namespace
