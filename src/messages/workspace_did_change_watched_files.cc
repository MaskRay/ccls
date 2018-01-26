#include "clang_complete.h"
#include "message_handler.h"
#include "project.h"
#include "queue_manager.h"
#include "working_files.h"

#include <loguru/loguru.hpp>

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
      std::string path = event.uri.GetPath();
      switch (event.type) {
        case lsFileChangeType::Created:
        // TODO
        case lsFileChangeType::Changed: {
          Project::Entry entry = project->FindCompilationEntryForFile(path);
          optional<std::string> content = ReadContent(path);
          if (!content)
            LOG_S(ERROR) << "Unable to read file content after saving " << path;
          else {
            bool is_interactive =
              working_files->GetFileByFilename(entry.filename) != nullptr;
            QueueManager::instance()->index_request.Enqueue(
                Index_Request(path, entry.args, is_interactive, *content));
            clang_complete->NotifySave(path);
          }
          break;
        }
        case lsFileChangeType::Deleted:
          // TODO
          break;
      }
    }
  }
};
REGISTER_MESSAGE_HANDLER(WorkspaceDidChangeWatchedFilesHandler);
}  // namespace
