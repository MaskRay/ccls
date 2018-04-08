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
    auto& params = request->params;
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        params.textDocument.uri.GetPath(), &file))
      return;

    WorkingFile* wfile =
        working_files->GetFileByFilename(file->def->path);

    Out_TextDocumentReferences out;
    out.id = request->id;
    bool container = g_config->xref.container;

    for (SymbolRef sym : FindSymbolsAtLocation(wfile, file, params.position)) {
      // Found symbol. Return references.
      EachOccurrenceWithParent(
          db, sym, params.context.includeDeclaration,
          [&](Use use, lsSymbolKind parent_kind) {
            if (use.role & params.context.role)
              if (std::optional<lsLocationEx> ls_loc =
                      GetLsLocationEx(db, working_files, use, container)) {
                if (container)
                  ls_loc->parentKind = parent_kind;
                out.result.push_back(*ls_loc);
              }
          });
      break;
    }

    if (out.result.empty()) {
      // |path| is the #include line. If the cursor is not on such line but line
      // = 0,
      // use the current filename.
      std::string path;
      if (params.position.line == 0)
        path = file->def->path;
      for (const IndexInclude& include : file->def->includes)
        if (include.line == params.position.line) {
          path = include.resolved_path;
          break;
        }
      if (path.size())
        for (QueryFile& file1 : db->files)
          if (file1.def)
            for (const IndexInclude& include : file1.def->includes)
              if (include.resolved_path == path) {
                // Another file |file1| has the same include line.
                lsLocationEx result;
                result.uri = lsDocumentUri::FromPath(file1.def->path);
                result.range.start.line = result.range.end.line =
                  include.line;
                out.result.push_back(std::move(result));
                break;
              }
    }

    if ((int)out.result.size() >= g_config->xref.maxNum)
      out.result.resize(g_config->xref.maxNum);
    QueueManager::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentReferences);
}  // namespace
