#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
struct Ipc_CqueryDerived : public RequestMessage<Ipc_CqueryDerived> {
  const static IpcId kIpcId = IpcId::CqueryDerived;
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
    std::vector<SymbolRef> refs =
      FindSymbolsAtLocation(working_file, file, request->params.position);
    // A template definition may be a use of its primary template.
    // We want to get the definition instead of the use.
    // Order by |Definition| DESC, range size ASC.
    std::stable_sort(refs.begin(), refs.end(),
                     [](const SymbolRef& a, const SymbolRef& b) {
                       return (a.role & SymbolRole::Definition) >
                              (b.role & SymbolRole::Definition);
                     });
    for (const SymbolRef& ref : refs) {
      if (ref.idx.kind == SymbolKind::Type) {
        QueryType& type = db->types[ref.idx.idx];
        std::vector<QueryLocation> locations =
            ToQueryLocation(db, type.derived);
        out.result = GetLsLocations(db, working_files, locations);
        break;
      } else if (ref.idx.kind == SymbolKind::Func) {
        QueryFunc& func = db->funcs[ref.idx.idx];
        std::vector<QueryLocation> locations =
            ToQueryLocation(db, func.derived);
        out.result = GetLsLocations(db, working_files, locations);
        break;
      }
    }
    QueueManager::WriteStdout(IpcId::CqueryDerived, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryDerivedHandler);
}  // namespace
