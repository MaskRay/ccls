#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

#include <loguru.hpp>

namespace {
MethodType kMethodType = "textDocument/references";

struct In_TextDocumentReferences : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct lsReferenceContext {
    // Include the declaration of the current symbol.
    bool includeDeclaration;
    // Include references with these |Role| bits set.
    Role role = Role::All;
  };
  struct Params {
    lsTextDocumentIdentifier textDocument;
    lsPosition position;
    lsReferenceContext context;
  };

  Params params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentReferences::lsReferenceContext,
                    includeDeclaration,
                    role);
MAKE_REFLECT_STRUCT(In_TextDocumentReferences::Params,
                    textDocument,
                    position,
                    context);
MAKE_REFLECT_STRUCT(In_TextDocumentReferences, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentReferences);

struct Out_TextDocumentReferences
    : public lsOutMessage<Out_TextDocumentReferences> {
  lsRequestId id;
  std::vector<lsLocationEx> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentReferences, jsonrpc, id, result);

struct Handler_TextDocumentReferences
    : BaseMessageHandler<In_TextDocumentReferences> {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(In_TextDocumentReferences* request) override {
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_TextDocumentReferences out;
    out.id = request->id;
    bool container = config->xref.container;

    for (const SymbolRef& sym :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      // Found symbol. Return references.
      EachOccurrenceWithParent(
          db, sym, request->params.context.includeDeclaration,
          [&](Use use, lsSymbolKind parent_kind) {
            if (use.role & request->params.context.role)
              if (optional<lsLocationEx> ls_loc =
                      GetLsLocationEx(db, working_files, use, container)) {
                if (container)
                  ls_loc->parentKind = parent_kind;
                out.result.push_back(*ls_loc);
              }
          });
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
                  lsLocationEx result;
                  result.uri = lsDocumentUri::FromPath(file1.def->path);
                  result.range.start.line = result.range.end.line =
                      include1.line;
                  out.result.push_back(std::move(result));
                  break;
                }
          break;
        }

    if ((int)out.result.size() >= config->xref.maxNum)
      out.result.resize(config->xref.maxNum);
    QueueManager::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentReferences);
}  // namespace
