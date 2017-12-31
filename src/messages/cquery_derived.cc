#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
struct Ipc_CqueryDerived : public IpcMessage<Ipc_CqueryDerived> {
  const static IpcId kIpcId = IpcId::CqueryDerived;
  lsRequestId id;
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
    for (const SymbolRef& ref :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      if (ref.idx.kind == SymbolKind::Type) {
        QueryType& type = db->types[ref.idx.idx];
        std::vector<QueryLocation> locations =
            ToQueryLocation(db, type.derived);
        out.result = GetLsLocations(db, working_files, locations);
      } else if (ref.idx.kind == SymbolKind::Func) {
        QueryFunc& func = db->funcs[ref.idx.idx];
        std::vector<QueryLocation> locations =
            ToQueryLocation(db, func.derived);
        out.result = GetLsLocations(db, working_files, locations);
      }
    }
    QueueManager::WriteStdout(IpcId::CqueryDerived, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryDerivedHandler);
}  // namespace
