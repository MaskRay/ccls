#include "message_handler.h"

namespace {
struct Ipc_TextDocumentDidSave : public IpcMessage<Ipc_TextDocumentDidSave> {
  struct Params {
    // The document that was saved.
    lsTextDocumentIdentifier textDocument;

    // Optional the content when saved. Depends on the includeText value
    // when the save notifcation was requested.
    // std::string text;
  };

  const static IpcId kIpcId = IpcId::TextDocumentDidSave;
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidSave::Params, textDocument);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidSave, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDidSave);

struct TextDocumentDidSaveHandler
    : BaseMessageHandler<Ipc_TextDocumentDidSave> {
  void Run(Ipc_TextDocumentDidSave* request) override {
    std::string path = request->params.textDocument.uri.GetPath();
    if (ShouldIgnoreFileForIndexing(path))
      return;

    // Send out an index request, and copy the current buffer state so we
    // can update the cached index contents when the index is done.
    //
    // We also do not index if there is already an index request.
    //
    // TODO: Cancel outgoing index request. Might be tricky to make
    //       efficient since we have to cancel.
    //    - we could have an |atomic<int> active_cancellations| variable
    //      that all of the indexers check before accepting an index. if
    //      zero we don't slow down fast-path. if non-zero we acquire
    //      mutex and check to see if we should skip the current request.
    //      if so, ignore that index response.
    // TODO: send as priority request
    Project::Entry entry = project->FindCompilationEntryForFile(path);
    QueueManager::instance()->index_request.Enqueue(Index_Request(
        entry.filename, entry.args, true /*is_interactive*/, nullopt));

    clang_complete->NotifySave(path);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentDidSaveHandler);
}  // namespace
