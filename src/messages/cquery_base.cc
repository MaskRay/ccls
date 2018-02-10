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
    std::vector<SymbolRef> syms =
        FindSymbolsAtLocation(working_file, file, request->params.position);
    // A template definition may be a use of its primary template.
    // We want to get the definition instead of the use.
    // Order by |Definition| DESC, range size ASC.
    std::stable_sort(syms.begin(), syms.end(),
                     [](const SymbolRef& a, const SymbolRef& b) {
                       return (a.role & SymbolRole::Definition) >
                              (b.role & SymbolRole::Definition);
                     });
    for (SymbolRef sym : syms) {
      if (sym.kind == SymbolKind::Type) {
        QueryType& type = db->GetType(sym);
        if (type.def)
          out.result = GetLsLocations(db, working_files,
                                      ToUses(db, type.def->parents));
        break;
      } else if (sym.kind == SymbolKind::Func) {
        QueryFunc& func = db->GetFunc(sym);
        if (func.def)
          out.result = GetLsLocations(db, working_files,
                                      ToUses(db, func.def->base));
        break;
      }
    }
    QueueManager::WriteStdout(IpcId::CqueryBase, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryBaseHandler);
}  // namespace
