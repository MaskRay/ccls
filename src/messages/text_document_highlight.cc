#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
struct Ipc_TextDocumentDocumentHighlight
    : public RequestMessage<Ipc_TextDocumentDocumentHighlight> {
  const static IpcId kIpcId = IpcId::TextDocumentDocumentHighlight;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDocumentHighlight, id, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDocumentHighlight);

struct Out_TextDocumentDocumentHighlight
    : public lsOutMessage<Out_TextDocumentDocumentHighlight> {
  lsRequestId id;
  std::vector<lsDocumentHighlight> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentDocumentHighlight, jsonrpc, id, result);

struct TextDocumentDocumentHighlightHandler
    : BaseMessageHandler<Ipc_TextDocumentDocumentHighlight> {
  void Run(Ipc_TextDocumentDocumentHighlight* request) override {
    QueryFileId file_id;
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file,
                        &file_id)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_TextDocumentDocumentHighlight out;
    out.id = request->id;

    for (SymbolRef sym :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      // Found symbol. Return references to highlight.
      std::vector<Use> uses = GetUsesOfSymbol(db, sym, true);
      out.result.reserve(uses.size());
      for (Use use : uses) {
        if (db->GetFileId(use) != file_id)
          continue;

        optional<lsLocation> ls_location =
            GetLsLocation(db, working_files, use);
        if (!ls_location)
          continue;

        lsDocumentHighlight highlight;
        highlight.kind = lsDocumentHighlightKind::Text;
        highlight.range = ls_location->range;
        out.result.push_back(highlight);
      }
      break;
    }

    QueueManager::WriteStdout(IpcId::TextDocumentDocumentHighlight, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentDocumentHighlightHandler);
}  // namespace
