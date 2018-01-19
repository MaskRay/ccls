#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
struct lsDocumentSymbolParams {
  lsTextDocumentIdentifier textDocument;
};
MAKE_REFLECT_STRUCT(lsDocumentSymbolParams, textDocument);

struct Ipc_TextDocumentDocumentSymbol
    : public RequestMessage<Ipc_TextDocumentDocumentSymbol> {
  const static IpcId kIpcId = IpcId::TextDocumentDocumentSymbol;
  lsDocumentSymbolParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDocumentSymbol, id, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDocumentSymbol);

struct Out_TextDocumentDocumentSymbol
    : public lsOutMessage<Out_TextDocumentDocumentSymbol> {
  lsRequestId id;
  std::vector<lsSymbolInformation> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentDocumentSymbol, jsonrpc, id, result);

struct TextDocumentDocumentSymbolHandler
    : BaseMessageHandler<Ipc_TextDocumentDocumentSymbol> {
  void Run(Ipc_TextDocumentDocumentSymbol* request) override {
    Out_TextDocumentDocumentSymbol out;
    out.id = request->id;

    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    for (SymbolRef ref : file->def->outline) {
      optional<lsSymbolInformation> info =
          GetSymbolInfo(db, working_files, ref.idx, true /*use_short_name*/);
      if (!info)
        continue;

      optional<lsLocation> location = GetLsLocation(db, working_files, ref.loc);
      if (!location)
        continue;
      info->location = *location;
      out.result.push_back(*info);
    }

    QueueManager::WriteStdout(IpcId::TextDocumentDocumentSymbol, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentDocumentSymbolHandler);
}  // namespace
