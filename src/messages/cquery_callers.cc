#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
MethodType kMethodType = "$cquery/callers";

struct In_CqueryCallers : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_CqueryCallers, id, params);
REGISTER_IN_MESSAGE(In_CqueryCallers);

struct Handler_CqueryCallers : BaseMessageHandler<In_CqueryCallers> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_CqueryCallers* request) override {
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
      if (sym.kind == SymbolKind::Func) {
        QueryFunc& func = db->GetFunc(sym);
        std::vector<Use> uses = func.uses;
        for (Use func_ref : GetUsesForAllBases(db, func))
          uses.push_back(func_ref);
        for (Use func_ref : GetUsesForAllDerived(db, func))
          uses.push_back(func_ref);
        out.result =
            GetLsLocationExs(db, working_files, uses, config->xref.container,
                             config->xref.maxNum);
        break;
      }
    }
    QueueManager::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CqueryCallers);
}  // namespace
