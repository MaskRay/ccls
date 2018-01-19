#include "message_handler.h"
#include "queue_manager.h"

namespace {
enum class lsFileChangeType {
  Created = 1,
  Changed = 2,
  Deleted = 3,
};
MAKE_REFLECT_TYPE_PROXY(lsFileChangeType,
                        std::underlying_type<lsFileChangeType>::type);

struct lsFileEvent {
  lsDocumentUri uri;
  lsFileChangeType type;
};
MAKE_REFLECT_STRUCT(lsFileEvent, uri, type);

struct lsDidChangeWatchedFilesParams {
  std::vector<lsFileEvent> changes;
};
MAKE_REFLECT_STRUCT(lsDidChangeWatchedFilesParams, changes);

struct Ipc_WorkspaceDidChangeWatchedFiles
    : public NotificationMessage<Ipc_WorkspaceDidChangeWatchedFiles> {
  const static IpcId kIpcId = IpcId::WorkspaceDidChangeWatchedFiles;
  lsDidChangeWatchedFilesParams params;
};
MAKE_REFLECT_STRUCT(Ipc_WorkspaceDidChangeWatchedFiles, params);
REGISTER_IPC_MESSAGE(Ipc_WorkspaceDidChangeWatchedFiles);

struct WorkspaceDidChangeWatchedFilesHandler
    : BaseMessageHandler<Ipc_WorkspaceDidChangeWatchedFiles> {
  void Run(Ipc_WorkspaceDidChangeWatchedFiles* request) override {
    for (lsFileEvent& event : request->params.changes) {
      switch (event.type) {
        case lsFileChangeType::Created:
          // TODO
          break;
        case lsFileChangeType::Deleted:
          // TODO
          break;
        case lsFileChangeType::Changed:
          // TODO
          break;
      }
    }
  }
};
REGISTER_MESSAGE_HANDLER(WorkspaceDidChangeWatchedFilesHandler);
}  // namespace
