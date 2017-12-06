#include "message_handler.h"
#include "query_utils.h"

struct CqueryCallTreeInitialHandler
    : BaseMessageHandler<Ipc_CqueryCallTreeInitial> {
  void Run(Ipc_CqueryCallTreeInitial* request) override {
    QueryFile* file;
    if (!FindFileOrFail(db, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_CqueryCallTree out;
    out.id = request->id;

    for (const SymbolRef& ref :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      if (ref.idx.kind == SymbolKind::Func) {
        out.result =
            BuildInitialCallTree(db, working_files, QueryFuncId(ref.idx.idx));
        break;
      }
    }

    IpcManager::WriteStdout(IpcId::CqueryCallTreeInitial, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryCallTreeInitialHandler);

struct CqueryCallTreeExpandHandler
    : BaseMessageHandler<Ipc_CqueryCallTreeExpand> {
  void Run(Ipc_CqueryCallTreeExpand* request) override {
    Out_CqueryCallTree out;
    out.id = request->id;

    auto func_id = db->usr_to_func.find(request->params.usr);
    if (func_id != db->usr_to_func.end())
      out.result = BuildExpandCallTree(db, working_files, func_id->second);

    IpcManager::WriteStdout(IpcId::CqueryCallTreeExpand, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryCallTreeExpandHandler);
