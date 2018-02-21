#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"
#include "symbol.h"

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
      EachUse(db, sym, true, [&](Use use) {
        if (use.file != file_id)
          return;
        if (optional<lsLocation> ls_loc =
                GetLsLocation(db, working_files, use)) {
          lsDocumentHighlight highlight;
          highlight.range = ls_loc->range;
          if (use.role & Role::Write)
            highlight.kind = lsDocumentHighlightKind::Write;
          else if (use.role & Role::Read)
            highlight.kind = lsDocumentHighlightKind::Read;
          else
            highlight.kind = lsDocumentHighlightKind::Text;
          highlight.role = use.role;
          out.result.push_back(highlight);
        }
      });
      break;
    }

    QueueManager::WriteStdout(IpcId::TextDocumentDocumentHighlight, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentDocumentHighlightHandler);
}  // namespace
