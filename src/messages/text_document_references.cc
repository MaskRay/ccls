#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

#include <loguru.hpp>

namespace {
struct Ipc_TextDocumentReferences
    : public RequestMessage<Ipc_TextDocumentReferences> {
  const static IpcId kIpcId = IpcId::TextDocumentReferences;
  struct lsReferenceContext {
    // Include the declaration of the current symbol.
    bool includeDeclaration;
  };
  struct Params {
    lsTextDocumentIdentifier textDocument;
    lsPosition position;
    lsReferenceContext context;
  };

  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentReferences::lsReferenceContext,
                    includeDeclaration);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentReferences::Params,
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
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_TextDocumentReferences out;
    out.id = request->id;

    for (const SymbolRef& sym :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      // Found symbol. Return references.
      std::vector<Use> uses = GetUsesOfSymbol(
          db, sym, request->params.context.includeDeclaration);
      out.result.reserve(uses.size());
      for (Use use : uses) {
        optional<lsLocation> ls_location =
            GetLsLocation(db, working_files, use);
        if (ls_location)
          out.result.push_back(*ls_location);
      }
      break;
    }

    if (out.result.empty())
      for (const IndexInclude& include : file->def->includes)
        if (include.line == request->params.position.line) {
          // |include| is the line the cursor is on.
          for (QueryFile& file1 : db->files)
            if (file1.def)
              for (const IndexInclude& include1 : file1.def->includes)
                if (include1.resolved_path == include.resolved_path) {
                  // Another file |file1| has the same include line.
                  lsLocation result;
                  result.uri = lsDocumentUri::FromPath(file1.def->path);
                  result.range.start.line = result.range.end.line =
                      include1.line;
                  out.result.push_back(std::move(result));
                  break;
                }
          break;
        }

    QueueManager::WriteStdout(IpcId::TextDocumentReferences, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentReferencesHandler);
}  // namespace
