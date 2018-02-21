#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
struct Ipc_CqueryBase : public RequestMessage<Ipc_CqueryBase> {
  const static IpcId kIpcId = IpcId::CqueryBase;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryBase, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryBase);

struct CqueryBaseHandler : BaseMessageHandler<Ipc_CqueryBase> {
  void Run(Ipc_CqueryBase* request) override {
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
        if (const auto* def = db->GetType(sym).AnyDef())
          out.result =
              GetLsLocationExs(db, working_files, ToUses(db, def->parents),
                               config->xref.container, config->xref.maxNum);
        break;
      } else if (sym.kind == SymbolKind::Func) {
        if (const auto* def = db->GetFunc(sym).AnyDef())
          out.result =
              GetLsLocationExs(db, working_files, ToUses(db, def->base),
                               config->xref.container, config->xref.maxNum);
        break;
      }
    }
    QueueManager::WriteStdout(IpcId::CqueryBase, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryBaseHandler);
}  // namespace
