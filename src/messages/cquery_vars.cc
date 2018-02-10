#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
struct Ipc_CqueryVars : public RequestMessage<Ipc_CqueryVars> {
  const static IpcId kIpcId = IpcId::CqueryVars;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryVars, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryVars);

struct CqueryVarsHandler : BaseMessageHandler<Ipc_CqueryVars> {
  void Run(Ipc_CqueryVars* request) override {
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
      RawId idx = sym.Idx();
      switch (sym.kind) {
        default:
          break;
        case SymbolKind::Var: {
          QueryVar& var = db->GetVar(sym);
          if (!var.def || !var.def->variable_type)
            continue;
          idx = var.def->variable_type->id;
        }
        // fallthrough
        case SymbolKind::Type: {
          QueryType& type = db->types[idx];
          out.result =
              GetLsLocations(db, working_files, ToUses(db, type.instances));
          break;
        }
      }
    }
    QueueManager::WriteStdout(IpcId::CqueryVars, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryVarsHandler);
}  // namespace
