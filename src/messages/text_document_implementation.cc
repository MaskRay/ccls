#include "message_handler.h"
#include "query_utils.h"
#include "pipeline.hh"
using namespace ccls;

namespace {
MethodType kMethodType = "textDocument/implementation";

struct In_TextDocumentImplementation : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentImplementation, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentImplementation);

struct Handler_TextDocumentImplementation
    : BaseMessageHandler<In_TextDocumentImplementation> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_TextDocumentImplementation* request) override {
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
        out.result = GetLsLocationExs(db, working_files,
                                      GetTypeDeclarations(db, type.derived));
        break;
      } else if (sym.kind == SymbolKind::Func) {
        QueryFunc& func = db->GetFunc(sym);
        out.result = GetLsLocationExs(db, working_files,
                                      GetFuncDeclarations(db, func.derived));
        break;
      }
    }
    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentImplementation);
}  // namespace
