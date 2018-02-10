#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
struct Ipc_CqueryCallers : public RequestMessage<Ipc_CqueryCallers> {
  const static IpcId kIpcId = IpcId::CqueryCallers;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryCallers, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryCallers);

struct CqueryCallersHandler : BaseMessageHandler<Ipc_CqueryCallers> {
  void Run(Ipc_CqueryCallers* request) override {
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
      if (sym.kind == SymbolKind::Func) {
        QueryFunc& func = db->GetFunc(sym);
        std::vector<Use> uses = func.uses;
        for (Use func_ref : GetCallersForAllBaseFunctions(db, func))
          uses.push_back(func_ref);
        for (Use func_ref : GetCallersForAllDerivedFunctions(db, func))
          uses.push_back(func_ref);
        out.result = GetLsLocations(db, working_files, uses);
      }
    }
    QueueManager::WriteStdout(IpcId::CqueryCallers, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryCallersHandler);
}  // namespace
