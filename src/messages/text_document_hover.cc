#include "message_handler.h"
#include "query_utils.h"

struct TextDocumentHoverHandler : BaseMessageHandler<Ipc_TextDocumentHover> {
  void Run(Ipc_TextDocumentHover* request) override {
    QueryFile* file;
    if (!FindFileOrFail(db, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_TextDocumentHover out;
    out.id = request->id;

    for (const SymbolRef& ref :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      // Found symbol. Return hover.
      optional<lsRange> ls_range = GetLsRange(
          working_files->GetFileByFilename(file->def->path), ref.loc.range);
      if (!ls_range)
        continue;

      out.result.contents.value = GetHoverForSymbol(db, ref.idx);
      out.result.contents.language = file->def->language;

      out.result.range = *ls_range;
      break;
    }

    IpcManager::WriteStdout(IpcId::TextDocumentHover, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentHoverHandler);
