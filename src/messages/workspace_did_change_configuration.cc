#include "cache_manager.h"
#include "message_handler.h"
#include "project.h"
#include "queue_manager.h"
#include "working_files.h"

namespace {
struct lsDidChangeConfigurationParams {
  bool placeholder;
};
MAKE_REFLECT_STRUCT(lsDidChangeConfigurationParams, placeholder);

struct Ipc_WorkspaceDidChangeConfiguration
    : public NotificationMessage<Ipc_WorkspaceDidChangeConfiguration> {
  const static IpcId kIpcId = IpcId::WorkspaceDidChangeConfiguration;
  lsDidChangeConfigurationParams params;
};
MAKE_REFLECT_STRUCT(Ipc_WorkspaceDidChangeConfiguration, params);
REGISTER_IPC_MESSAGE(Ipc_WorkspaceDidChangeConfiguration);

struct WorkspaceDidChangeConfigurationHandler
    : BaseMessageHandler<Ipc_WorkspaceDidChangeConfiguration> {
  void Run(Ipc_WorkspaceDidChangeConfiguration* request) override {
  }
};
REGISTER_MESSAGE_HANDLER(WorkspaceDidChangeConfigurationHandler);
}  // namespace
