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
    for (const SymbolRef& ref :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      size_t id = ref.idx.idx;
      switch (ref.idx.kind) {
        default:
          break;
        case SymbolKind::Var: {
          QueryVar& var = db->vars[id];
          if (!var.def || !var.def->variable_type)
            continue;
          id = var.def->variable_type->id;
        }
        // fallthrough
        case SymbolKind::Type: {
          QueryType& type = db->types[id];
          out.result = GetLsLocations(db, working_files,
                                      ToReference(db, type.instances));
          break;
        }
      }
    }
    QueueManager::WriteStdout(IpcId::CqueryVars, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryVarsHandler);
}  // namespace
