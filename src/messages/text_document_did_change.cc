#include "cache_manager.h"
#include "clang_complete.h"
#include "message_handler.h"
#include "project.h"
#include "working_files.h"
#include "queue_manager.h"

#include <loguru/loguru.hpp>

namespace {
MethodType kMethodType = "textDocument/didChange";

struct In_TextDocumentDidChange : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsTextDocumentDidChangeParams params;
};

MAKE_REFLECT_STRUCT(In_TextDocumentDidChange, params);
REGISTER_IN_MESSAGE(In_TextDocumentDidChange);

struct Handler_TextDocumentDidChange
    : BaseMessageHandler<In_TextDocumentDidChange> {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(In_TextDocumentDidChange* request) override {
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
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDidChange);
}  // namespace
