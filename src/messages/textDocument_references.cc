#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
using namespace ccls;

#include <unordered_set>

namespace {
MethodType kMethodType = "textDocument/references";

struct In_TextDocumentReferences : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct lsReferenceContext {
    bool base = true;
    // Exclude references with any |Role| bits set.
    Role excludeRole = Role::None;
    // Include the declaration of the current symbol.
    bool includeDeclaration = false;
    // Include references with all |Role| bits set.
    Role role = Role::None;
  };
  struct Params {
    lsTextDocumentIdentifier textDocument;
    lsPosition position;
    lsReferenceContext context;
  };

  Params params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentReferences::lsReferenceContext,
                    base,
                    excludeRole,
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
      std::unordered_set<Usr> seen;
      seen.insert(sym.usr);
      std::vector<Usr> stack{sym.usr};
      if (sym.kind != SymbolKind::Func)
        params.context.base = false;
      while (stack.size()) {
        sym.usr = stack.back();
        stack.pop_back();
        auto fn = [&](Use use, lsSymbolKind parent_kind) {
          if (Role(use.role & params.context.role) == params.context.role &&
              !(use.role & params.context.excludeRole))
            if (std::optional<lsLocationEx> ls_loc =
                    GetLsLocationEx(db, working_files, use, container)) {
              if (container)
                ls_loc->parentKind = parent_kind;
              out.result.push_back(*ls_loc);
            }
        };
        WithEntity(db, sym, [&](const auto& entity) {
          lsSymbolKind parent_kind = lsSymbolKind::Unknown;
          for (auto& def : entity.def)
            if (def.spell) {
              parent_kind = GetSymbolKind(db, sym);
              if (params.context.base)
                for (Usr usr : def.GetBases())
                  if (!seen.count(usr)) {
                    seen.insert(usr);
                    stack.push_back(usr);
                  }
              break;
            }
          for (Use use : entity.uses)
            fn(use, parent_kind);
          if (params.context.includeDeclaration) {
            for (auto& def : entity.def)
              if (def.spell)
                fn(*def.spell, parent_kind);
            for (Use use : entity.declarations)
              fn(use, parent_kind);
          }
        });
      }
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
    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentReferences);
}  // namespace
