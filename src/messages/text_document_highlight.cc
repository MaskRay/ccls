#include "message_handler.h"
#include "query_utils.h"

namespace {
struct Ipc_TextDocumentDocumentHighlight
    : public IpcMessage<Ipc_TextDocumentDocumentHighlight> {
  const static IpcId kIpcId = IpcId::TextDocumentDocumentHighlight;

  lsRequestId id;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDocumentHighlight, id, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDocumentHighlight);

struct Out_TextDocumentDocumentHighlight
    : public lsOutMessage<Out_TextDocumentDocumentHighlight> {
  lsRequestId id;
  NonElidedVector<lsDocumentHighlight> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentDocumentHighlight, jsonrpc, id, result);

struct TextDocumentDocumentHighlightHandler
    : BaseMessageHandler<Ipc_TextDocumentDocumentHighlight> {
  void Run(Ipc_TextDocumentDocumentHighlight* request) override {
    QueryFileId file_id;
    QueryFile* file;
    if (!FindFileOrFail(db, request->id,
                        request->params.textDocument.uri.GetPath(), &file,
                        &file_id)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_TextDocumentDocumentHighlight out;
    out.id = request->id;

    for (const SymbolRef& ref :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      // Found symbol. Return references to highlight.
      std::vector<QueryLocation> uses = GetUsesOfSymbol(db, ref.idx);
      out.result.reserve(uses.size());
      for (const QueryLocation& use : uses) {
        if (use.path != file_id)
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

    IpcManager::WriteStdout(IpcId::TextDocumentDocumentHighlight, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentDocumentHighlightHandler);
}  // namespace