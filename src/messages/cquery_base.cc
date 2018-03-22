#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {

MethodType kMethodType = "$cquery/base";

struct In_CqueryBase : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }

  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_CqueryBase, id, params);
REGISTER_IN_MESSAGE(In_CqueryBase);

struct Handler_CqueryBase : BaseMessageHandler<In_CqueryBase> {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(In_CqueryBase* request) override {
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
        if (const auto* def = db->GetType(sym).AnyDef())
          out.result = GetLsLocationExs(
              db, working_files, GetDeclarations(db, def->bases),
              config->xref.container, config->xref.maxNum);
        break;
      } else if (sym.kind == SymbolKind::Func) {
        if (const auto* def = db->GetFunc(sym).AnyDef())
          out.result = GetLsLocationExs(
              db, working_files, GetDeclarations(db, def->bases),
              config->xref.container, config->xref.maxNum);
        break;
      }
    }
    QueueManager::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CqueryBase);
}  // namespace
