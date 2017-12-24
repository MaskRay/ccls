#include "message_handler.h"
#include "query_utils.h"

namespace {
struct Ipc_CqueryVars : public IpcMessage<Ipc_CqueryVars> {
  const static IpcId kIpcId = IpcId::CqueryVars;
  lsRequestId id;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryVars, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryVars);

struct CqueryVarsHandler : BaseMessageHandler<Ipc_CqueryVars> {
  void Run(Ipc_CqueryVars* request) override {
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
      if (ref.idx.kind == SymbolKind::Type) {
        QueryType& type = db->types[ref.idx.idx];
        std::vector<QueryLocation> locations =
            ToQueryLocation(db, type.instances);
        out.result = GetLsLocations(db, working_files, locations);
      }
    }
    QueueManager::WriteStdout(IpcId::CqueryVars, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryVarsHandler);
}  // namespace