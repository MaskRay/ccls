#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
using namespace ccls;

namespace {

MethodType kMethodType = "$ccls/base";

struct In_CclsBase : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }

  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_CclsBase, id, params);
REGISTER_IN_MESSAGE(In_CclsBase);

struct Handler_CclsBase : BaseMessageHandler<In_CclsBase> {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(In_CclsBase* request) override {
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
          out.result = GetLsLocationExs(db, working_files,
                                        GetTypeDeclarations(db, def->bases));
        break;
      } else if (sym.kind == SymbolKind::Func) {
        if (const auto* def = db->GetFunc(sym).AnyDef())
          out.result = GetLsLocationExs(db, working_files,
                                        GetFuncDeclarations(db, def->bases));
        break;
      }
    }
    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CclsBase);
}  // namespace
