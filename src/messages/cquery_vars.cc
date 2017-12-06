#include "message_handler.h"
#include "query_utils.h"

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
    IpcManager::WriteStdout(IpcId::CqueryVars, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryVarsHandler);