#include "message_handler.h"
#include "query_utils.h"

#include <loguru.hpp>

namespace {
struct Ipc_TextDocumentReferences
    : public IpcMessage<Ipc_TextDocumentReferences> {
  struct lsReferenceContext {
    // Include the declaration of the current symbol.
    bool includeDeclaration;
  };
  struct lsReferenceParams : public lsTextDocumentPositionParams {
    lsTextDocumentIdentifier textDocument;
    lsPosition position;
    lsReferenceContext context;
  };

  const static IpcId kIpcId = IpcId::TextDocumentReferences;

  lsRequestId id;
  lsReferenceParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentReferences::lsReferenceContext,
                    includeDeclaration);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentReferences::lsReferenceParams,
                    textDocument,
                    position,
                    context);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentReferences, id, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentReferences);

struct Out_TextDocumentReferences
    : public lsOutMessage<Out_TextDocumentReferences> {
  lsRequestId id;
  std::vector<lsLocation> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentReferences, jsonrpc, id, result);

struct TextDocumentReferencesHandler
    : BaseMessageHandler<Ipc_TextDocumentReferences> {
  void Run(Ipc_TextDocumentReferences* request) override {
    QueryFile* file;
    if (!FindFileOrFail(db, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_TextDocumentReferences out;
    out.id = request->id;

    for (const SymbolRef& ref :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      optional<QueryLocation> excluded_declaration;
      if (!request->params.context.includeDeclaration) {
        LOG_S(INFO) << "Excluding declaration in references";
        excluded_declaration = GetDefinitionSpellingOfSymbol(db, ref.idx);
      }

      // Found symbol. Return references.
      std::vector<QueryLocation> uses = GetUsesOfSymbol(db, ref.idx);
      out.result.reserve(uses.size());
      for (const QueryLocation& use : uses) {
        if (excluded_declaration.has_value() && use == *excluded_declaration)
          continue;

        optional<lsLocation> ls_location =
            GetLsLocation(db, working_files, use);
        if (ls_location)
          out.result.push_back(*ls_location);
      }
      break;
    }

    QueueManager::WriteStdout(IpcId::TextDocumentReferences, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentReferencesHandler);
}  // namespace