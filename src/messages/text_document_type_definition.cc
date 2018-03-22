#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
MethodType kMethodType = "textDocument/typeDefinition";

struct In_TextDocumentTypeDefinition : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentTypeDefinition, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentTypeDefinition);

struct Out_TextDocumentTypeDefinition
    : public lsOutMessage<Out_TextDocumentTypeDefinition> {
  lsRequestId id;
  std::vector<lsLocationEx> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentTypeDefinition, jsonrpc, id, result);

struct Handler_TextDocumentTypeDefinition
    : BaseMessageHandler<In_TextDocumentTypeDefinition> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_TextDocumentTypeDefinition* request) override {
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file,
                        nullptr)) {
      return;
    }
    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_TextDocumentTypeDefinition out;
    out.id = request->id;
    for (SymbolRef sym :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      Id<void> id = sym.id;
      switch (sym.kind) {
        case SymbolKind::Var: {
          const QueryVar::Def* def = db->GetVar(sym).AnyDef();
          if (!def || !def->type)
            continue;
          id = *def->type;
        }
          // fallthrough
        case SymbolKind::Type: {
          QueryType& type = db->types[id.id];
          for (const auto& def : type.def)
            if (def.spell) {
              if (auto ls_loc = GetLsLocationEx(db, working_files, *def.spell,
                                                config->xref.container))
                out.result.push_back(*ls_loc);
            }
          break;
        }
        default:
          break;
      }
    }

    QueueManager::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentTypeDefinition);

}  // namespace
