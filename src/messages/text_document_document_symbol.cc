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
    QueryFileId file_id;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file,
                        &file_id)) {
      return;
    }

    for (SymbolRef sym : file->def->outline) {
      optional<lsSymbolInformation> info =
          GetSymbolInfo(db, working_files, sym, true /*use_short_name*/);
      if (!info)
        continue;
      if (sym.kind == SymbolKind::Var) {
        QueryVar& var = db->GetVar(sym);
        auto* def = var.AnyDef();
        if (!def || !def->spell)
          continue;
        // Ignore local variables.
        if (def->spell->kind == SymbolKind::Func &&
            def->storage != StorageClass::Static &&
            def->storage != StorageClass::Extern)
          continue;
      }

      if (optional<lsLocation> location = GetLsLocation(
              db, working_files,
              Use(sym.range, sym.id, sym.kind, sym.role, file_id))) {
        info->location = *location;
        out.result.push_back(*info);
      }
    }

    QueueManager::WriteStdout(IpcId::TextDocumentDocumentSymbol, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentDocumentSymbolHandler);
}  // namespace
