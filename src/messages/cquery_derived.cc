#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
MethodType kMethodType = "$cquery/derived";

struct In_CqueryDerived : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_CqueryDerived, id, params);
REGISTER_IN_MESSAGE(In_CqueryDerived);

struct Handler_CqueryDerived : BaseMessageHandler<In_CqueryDerived> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_CqueryDerived* request) override {
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
      if (sym.kind == SymbolKind::Type) {
        QueryType& type = db->GetType(sym);
        out.result = GetLsLocationExs(
            db, working_files, GetDeclarations(db, type.derived),
            config->xref.container, config->xref.maxNum);
        break;
      } else if (sym.kind == SymbolKind::Func) {
        QueryFunc& func = db->GetFunc(sym);
        out.result = GetLsLocationExs(
            db, working_files, GetDeclarations(db, func.derived),
            config->xref.container, config->xref.maxNum);
        break;
      }
    }
    QueueManager::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CqueryDerived);
}  // namespace
