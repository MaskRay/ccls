#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
struct Ipc_CqueryDerived : public RequestMessage<Ipc_CqueryDerived> {
  const static IpcId kIpcId = IpcId::CqueryDerived;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryDerived, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryDerived);

struct CqueryDerivedHandler : BaseMessageHandler<Ipc_CqueryDerived> {
  void Run(Ipc_CqueryDerived* request) override {
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_LocationList out;
    out.id = request->id;
    for (SymbolRef sym :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      if (sym.kind == SymbolKind::Type) {
        QueryType& type = db->GetType(sym);
        out.result =
            GetLsLocationExs(db, working_files, ToUses(db, type.derived),
                             config->xref.container, config->xref.maxNum);
        break;
      } else if (sym.kind == SymbolKind::Func) {
        QueryFunc& func = db->GetFunc(sym);
        out.result =
            GetLsLocationExs(db, working_files, ToUses(db, func.derived),
                             config->xref.container, config->xref.maxNum);
        break;
      }
    }
    QueueManager::WriteStdout(IpcId::CqueryDerived, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryDerivedHandler);
}  // namespace
