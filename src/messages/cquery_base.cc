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
    for (const SymbolRef& ref :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      if (ref.idx.kind == SymbolKind::Type) {
        QueryType& type = db->types[ref.idx.idx];
        if (!type.def)
          continue;
        std::vector<QueryLocation> locations =
            ToQueryLocation(db, type.def->parents);
        out.result = GetLsLocations(db, working_files, locations);
      } else if (ref.idx.kind == SymbolKind::Func) {
        QueryFunc& func = db->funcs[ref.idx.idx];
        std::vector<QueryLocation> locations =
            ToQueryLocation(db, func.def->base);
        out.result = GetLsLocations(db, working_files, locations);
      }
    }
    QueueManager::WriteStdout(IpcId::CqueryBase, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryBaseHandler);
}  // namespace
