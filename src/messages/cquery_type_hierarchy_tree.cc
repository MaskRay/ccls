#include "message_handler.h"
#include "query_utils.h"

struct CqueryTypeHierarchyTreeHandler
    : BaseMessageHandler<Ipc_CqueryTypeHierarchyTree> {
  void Run(Ipc_CqueryTypeHierarchyTree* request) override {
    QueryFile* file;
    if (!FindFileOrFail(db, request->id,
                        request->params.textDocument.uri.GetPath(), &file))
      return;

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_CqueryTypeHierarchyTree out;
    out.id = request->id;

    for (const SymbolRef& ref :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      if (ref.idx.kind == SymbolKind::Type) {
        out.result = BuildInheritanceHierarchyForType(db, working_files,
                                                      QueryTypeId(ref.idx.idx));
        break;
      }
      if (ref.idx.kind == SymbolKind::Func) {
        out.result = BuildInheritanceHierarchyForFunc(db, working_files,
                                                      QueryFuncId(ref.idx.idx));
        break;
      }
    }

    IpcManager::WriteStdout(IpcId::CqueryTypeHierarchyTree, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryTypeHierarchyTreeHandler);