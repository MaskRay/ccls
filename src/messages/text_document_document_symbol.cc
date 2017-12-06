#include "message_handler.h"
#include "query_utils.h"

struct TextDocumentDocumentSymbolHandler
    : BaseMessageHandler<Ipc_TextDocumentDocumentSymbol> {
  void Run(Ipc_TextDocumentDocumentSymbol* request) override {
    Out_TextDocumentDocumentSymbol out;
    out.id = request->id;

    QueryFile* file;
    if (!FindFileOrFail(db, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    for (SymbolRef ref : file->def->outline) {
      optional<lsSymbolInformation> info =
          GetSymbolInfo(db, working_files, ref.idx);
      if (!info)
        continue;

      optional<lsLocation> location = GetLsLocation(db, working_files, ref.loc);
      if (!location)
        continue;
      info->location = *location;
      out.result.push_back(*info);
    }

    IpcManager::WriteStdout(IpcId::TextDocumentDocumentSymbol, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentDocumentSymbolHandler);
