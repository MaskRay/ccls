#include "message_handler.h"
#include "query_utils.h"

struct TextDocumentRenameHandler : BaseMessageHandler<Ipc_TextDocumentRename> {
  void Run(Ipc_TextDocumentRename* request) override {
    QueryFileId file_id;
    QueryFile* file;
    if (!FindFileOrFail(db, request->id,
                        request->params.textDocument.uri.GetPath(), &file,
                        &file_id)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_TextDocumentRename out;
    out.id = request->id;

    for (const SymbolRef& ref :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      // Found symbol. Return references to rename.
      std::vector<QueryLocation> uses = GetUsesOfSymbol(db, ref.idx);
      out.result =
          BuildWorkspaceEdit(db, working_files, uses, request->params.newName);
      break;
    }

    IpcManager::WriteStdout(IpcId::TextDocumentRename, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentRenameHandler);
