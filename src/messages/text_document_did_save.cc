#include "cache_manager.h"
#include "clang_complete.h"
#include "message_handler.h"
#include "project.h"
#include "queue_manager.h"

#include <loguru/loguru.hpp>

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

  void Run(In_TextDocumentDidSave* request) override {
    std::string path = request->params.textDocument.uri.GetPath();
    if (ShouldIgnoreFileForIndexing(path))
      return;

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
    if (!config->enableIndexOnDidChange) {
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

    clang_complete->NotifySave(path);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDidSave);
}  // namespace
