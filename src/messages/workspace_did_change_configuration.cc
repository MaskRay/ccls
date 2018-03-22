#include "cache_manager.h"
#include "clang_complete.h"
#include "message_handler.h"
#include "project.h"
#include "queue_manager.h"
#include "timer.h"
#include "working_files.h"

#include <loguru.hpp>

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
  void Run(In_WorkspaceDidChangeConfiguration* request) override {
    Timer time;
    project->Load(config, config->projectRoot);
    time.ResetAndPrint("[perf] Loaded compilation entries (" +
                       std::to_string(project->entries.size()) + " files)");

    time.Reset();
    project->Index(config, QueueManager::instance(), working_files,
                   std::monostate());
    time.ResetAndPrint(
        "[perf] Dispatched workspace/didChangeConfiguration index requests");

    clang_complete->FlushAllSessions();
    LOG_S(INFO) << "Flushed all clang complete sessions";
  }
};
REGISTER_MESSAGE_HANDLER(Handler_WorkspaceDidChangeConfiguration);
}  // namespace
