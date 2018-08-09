#include "clang_complete.h"
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
