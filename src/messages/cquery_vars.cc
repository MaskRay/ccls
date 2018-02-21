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
      Id<void> id = sym.id;
      switch (sym.kind) {
        default:
          break;
        case SymbolKind::Var: {
          const QueryVar::Def* def = db->GetVar(sym).AnyDef();
          if (!def || !def->type)
            continue;
          id = *def->type;
        }
        // fallthrough
        case SymbolKind::Type: {
          QueryType& type = db->types[id.id];
          out.result =
              GetLsLocationExs(db, working_files, ToUses(db, type.instances),
                               config->xref.container, config->xref.maxNum);
          break;
        }
      }
    }
    QueueManager::WriteStdout(IpcId::CqueryVars, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryVarsHandler);
}  // namespace
