#include "message_handler.h"
#include "query_utils.h"

namespace {
struct Ipc_CqueryCallers : public IpcMessage<Ipc_CqueryCallers> {
  const static IpcId kIpcId = IpcId::CqueryCallers;
  lsRequestId id;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryCallers, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryCallers);

struct CqueryCallersHandler : BaseMessageHandler<Ipc_CqueryCallers> {
  void Run(Ipc_CqueryCallers* request) override {
    QueryFile* file;
    if (!FindFileOrFail(db, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_LocationList out;
    out.id = request->id;
    for (const SymbolRef& ref :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      if (ref.idx.kind == SymbolKind::Func) {
        QueryFunc& func = db->funcs[ref.idx.idx];
        std::vector<QueryLocation> locations =
            ToQueryLocation(db, func.callers);
        for (QueryFuncRef func_ref : GetCallersForAllBaseFunctions(db, func))
          locations.push_back(func_ref.loc);
        for (QueryFuncRef func_ref : GetCallersForAllDerivedFunctions(db, func))
          locations.push_back(func_ref.loc);

        out.result = GetLsLocations(db, working_files, locations);
      }
    }
    QueueManager::WriteStdout(IpcId::CqueryCallers, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryCallersHandler);
}  // namespace