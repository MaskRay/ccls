#include "message_handler.h"
#include "query_utils.h"

namespace {
struct Ipc_TextDocumentRename : public IpcMessage<Ipc_TextDocumentRename> {
  struct Params {
    // The document to format.
    lsTextDocumentIdentifier textDocument;

    // The position at which this request was sent.
    lsPosition position;

    // The new name of the symbol. If the given name is not valid the
    // request must return a [ResponseError](#ResponseError) with an
    // appropriate message set.
    std::string newName;
  };
  const static IpcId kIpcId = IpcId::TextDocumentRename;

  lsRequestId id;
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentRename::Params,
                    textDocument,
                    position,
                    newName);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentRename, id, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentRename);

struct Out_TextDocumentRename : public lsOutMessage<Out_TextDocumentRename> {
  lsRequestId id;
  lsWorkspaceEdit result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentRename, jsonrpc, id, result);

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
}  // namespace