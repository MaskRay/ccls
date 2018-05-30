#include "message_handler.h"
#include "query_utils.h"
#include "pipeline.hh"
using namespace ccls;

namespace {
MethodType kMethodType = "$ccls/vars";

struct In_CclsVars : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }

  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_CclsVars, id, params);
REGISTER_IN_MESSAGE(In_CclsVars);

struct Handler_CclsVars : BaseMessageHandler<In_CclsVars> {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(In_CclsVars* request) override {
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
      Usr usr = sym.usr;
      switch (sym.kind) {
        default:
          break;
        case SymbolKind::Var: {
          const QueryVar::Def* def = db->GetVar(sym).AnyDef();
          if (!def || !def->type)
            continue;
          usr = def->type;
          [[fallthrough]];
        }
        case SymbolKind::Type:
          out.result =
              GetLsLocationExs(db, working_files,
                               GetVarDeclarations(db, db->Type(usr).instances));
          break;
      }
    }
    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CclsVars);
}  // namespace
