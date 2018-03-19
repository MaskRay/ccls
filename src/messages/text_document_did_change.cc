#include "cache_manager.h"
#include "clang_complete.h"
#include "message_handler.h"
#include "project.h"
#include "working_files.h"
#include "queue_manager.h"

#include <loguru/loguru.hpp>

namespace {
struct Ipc_TextDocumentDidChange
    : public NotificationMessage<Ipc_TextDocumentDidChange> {
  const static IpcId kIpcId = IpcId::TextDocumentDidChange;
  lsTextDocumentDidChangeParams params;
};

MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidChange, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDidChange);

struct TextDocumentDidChangeHandler
    : BaseMessageHandler<Ipc_TextDocumentDidChange> {
  void Run(Ipc_TextDocumentDidChange* request) override {
    std::string path = request->params.textDocument.uri.GetPath();
    working_files->OnChange(request->params);
    if (config->enableIndexOnDidChange) {
      optional<std::string> content = ReadContent(path);
      if (!content) {
        LOG_S(ERROR) << "Unable to read file content after saving " << path;
      } else {
        Project::Entry entry = project->FindCompilationEntryForFile(path);
        QueueManager::instance()->index_request.PushBack(
            Index_Request(entry.filename, entry.args, true /*is_interactive*/,
                          *content, ICacheManager::Make(config)),
            true);
      }
    }
    clang_complete->NotifyEdit(path);
    clang_complete->DiagnosticsUpdate(
        std::monostate(),
        request->params.textDocument.AsTextDocumentIdentifier());
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentDidChangeHandler);
}  // namespace
