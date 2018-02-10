#include "clang_complete.h"
#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
struct lsDocumentCodeLensParams {
  lsTextDocumentIdentifier textDocument;
};
MAKE_REFLECT_STRUCT(lsDocumentCodeLensParams, textDocument);

struct lsCodeLensUserData {};
MAKE_REFLECT_EMPTY_STRUCT(lsCodeLensUserData);

struct lsCodeLensCommandArguments {
  lsDocumentUri uri;
  lsPosition position;
  std::vector<lsLocation> locations;
};
void Reflect(Writer& visitor, lsCodeLensCommandArguments& value) {
  visitor.StartArray(3);
  Reflect(visitor, value.uri);
  Reflect(visitor, value.position);
  Reflect(visitor, value.locations);
  visitor.EndArray();
}
#if false
void Reflect(Reader& visitor, lsCodeLensCommandArguments& value) {
  auto it = visitor.Begin();
  Reflect(*it, value.uri);
  ++it;
  Reflect(*it, value.position);
  ++it;
  Reflect(*it, value.locations);
}
#endif

using TCodeLens = lsCodeLens<lsCodeLensUserData, lsCodeLensCommandArguments>;
struct Ipc_TextDocumentCodeLens
    : public RequestMessage<Ipc_TextDocumentCodeLens> {
  const static IpcId kIpcId = IpcId::TextDocumentCodeLens;
  lsDocumentCodeLensParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentCodeLens, id, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentCodeLens);

struct Out_TextDocumentCodeLens
    : public lsOutMessage<Out_TextDocumentCodeLens> {
  lsRequestId id;
  std::vector<lsCodeLens<lsCodeLensUserData, lsCodeLensCommandArguments>>
      result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentCodeLens, jsonrpc, id, result);

#if false
struct Ipc_CodeLensResolve : public IpcMessage<Ipc_CodeLensResolve> {
  const static IpcId kIpcId = IpcId::CodeLensResolve;

  lsRequestId id;
  TCodeLens params;
};
MAKE_REFLECT_STRUCT(Ipc_CodeLensResolve, id, params);
REGISTER_IPC_MESSAGE(Ipc_CodeLensResolve);

struct Out_CodeLensResolve : public lsOutMessage<Out_CodeLensResolve> {
  lsRequestId id;
  TCodeLens result;
};
MAKE_REFLECT_STRUCT(Out_CodeLensResolve, jsonrpc, id, result);
#endif

struct CommonCodeLensParams {
  std::vector<TCodeLens>* result;
  QueryDatabase* db;
  WorkingFiles* working_files;
  WorkingFile* working_file;
};

SymbolRef OffsetStartColumn(SymbolRef sym, int16_t offset) {
  sym.range.start.column += offset;
  return sym;
}

void AddCodeLens(const char* singular,
                 const char* plural,
                 CommonCodeLensParams* common,
                 SymbolRef loc,
                 const std::vector<Use>& uses,
                 bool force_display) {
  TCodeLens code_lens;
  optional<lsRange> range = GetLsRange(common->working_file, loc.range);
  if (!range)
    return;
  code_lens.range = *range;
  code_lens.command = lsCommand<lsCodeLensCommandArguments>();
  code_lens.command->command = "cquery.showReferences";
  code_lens.command->arguments.uri =
      GetLsDocumentUri(common->db, common->db->GetFileId(loc));
  code_lens.command->arguments.position = code_lens.range.start;

  // Add unique uses.
  std::unordered_set<lsLocation> unique_uses;
  for (Use use : uses) {
    optional<lsLocation> location =
        GetLsLocation(common->db, common->working_files, use);
    if (!location)
      continue;
    unique_uses.insert(*location);
  }
  code_lens.command->arguments.locations.assign(unique_uses.begin(),
                                                unique_uses.end());

  // User visible label
  size_t num_usages = unique_uses.size();
  code_lens.command->title = std::to_string(num_usages) + " ";
  if (num_usages == 1)
    code_lens.command->title += singular;
  else
    code_lens.command->title += plural;

  if (force_display || unique_uses.size() > 0)
    common->result->push_back(code_lens);
}

struct TextDocumentCodeLensHandler
    : BaseMessageHandler<Ipc_TextDocumentCodeLens> {
  void Run(Ipc_TextDocumentCodeLens* request) override {
    Out_TextDocumentCodeLens out;
    out.id = request->id;

    lsDocumentUri file_as_uri = request->params.textDocument.uri;
    std::string path = file_as_uri.GetPath();

    clang_complete->NotifyView(path);

    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    CommonCodeLensParams common;
    common.result = &out.result;
    common.db = db;
    common.working_files = working_files;
    common.working_file = working_files->GetFileByFilename(file->def->path);

    for (SymbolRef sym : file->def->outline) {
      // NOTE: We OffsetColumn so that the code lens always show up in a
      // predictable order. Otherwise, the client may randomize it.

      switch (sym.kind) {
        case SymbolKind::Type: {
          QueryType& type = db->GetType(sym);
          if (!type.def)
            continue;
          if (type.def->kind == ClangSymbolKind::Namespace)
            continue;
          AddCodeLens("ref", "refs", &common, OffsetStartColumn(sym, 0),
                      type.uses, true /*force_display*/);
          AddCodeLens("derived", "derived", &common, OffsetStartColumn(sym, 1),
                      ToUses(db, type.derived), false /*force_display*/);
          AddCodeLens("var", "vars", &common, OffsetStartColumn(sym, 2),
                      ToUses(db, type.instances), false /*force_display*/);
          break;
        }
        case SymbolKind::Func: {
          QueryFunc& func = db->GetFunc(sym);
          if (!func.def)
            continue;

          int16_t offset = 0;

          // For functions, the outline will report a location that is using the
          // extent since that is better for outline. This tries to convert the
          // extent location to the spelling location.
          auto try_ensure_spelling = [&](SymbolRef sym) {
            Maybe<Reference> def = GetDefinitionSpellingOfSymbol(db, sym);
            if (!def || db->GetFileId(*def) != db->GetFileId(sym) ||
                def->range.start.line != sym.range.start.line) {
              return sym;
            }
            return SymbolRef(*def);
          };

          std::vector<Use> base_callers =
              GetCallersForAllBaseFunctions(db, func);
          std::vector<Use> derived_callers =
              GetCallersForAllDerivedFunctions(db, func);
          if (base_callers.empty() && derived_callers.empty()) {
            SymbolRef loc = try_ensure_spelling(sym);
            AddCodeLens("call", "calls", &common,
                        OffsetStartColumn(loc, offset++), func.uses,
                        true /*force_display*/);
          } else {
            SymbolRef loc = try_ensure_spelling(sym);
            AddCodeLens("direct call", "direct calls", &common,
                        OffsetStartColumn(loc, offset++), func.uses,
                        false /*force_display*/);
            if (!base_callers.empty())
              AddCodeLens("base call", "base calls", &common,
                          OffsetStartColumn(loc, offset++), base_callers,
                          false /*force_display*/);
            if (!derived_callers.empty())
              AddCodeLens("derived call", "derived calls", &common,
                          OffsetStartColumn(loc, offset++), derived_callers,
                          false /*force_display*/);
          }

          AddCodeLens("derived", "derived", &common,
                      OffsetStartColumn(sym, offset++),
                      ToUses(db, func.derived), false /*force_display*/);

          // "Base"
          if (func.def->base.size() == 1) {
            Maybe<Reference> base_loc =
                GetDefinitionSpellingOfSymbol(db, func.def->base[0]);
            if (base_loc) {
              optional<lsLocation> ls_base =
                  GetLsLocation(db, working_files, *base_loc);
              if (ls_base) {
                optional<lsRange> range =
                    GetLsRange(common.working_file, sym.range);
                if (range) {
                  TCodeLens code_lens;
                  code_lens.range = *range;
                  code_lens.range.start.character += offset++;
                  code_lens.command = lsCommand<lsCodeLensCommandArguments>();
                  code_lens.command->title = "Base";
                  code_lens.command->command = "cquery.goto";
                  code_lens.command->arguments.uri = ls_base->uri;
                  code_lens.command->arguments.position = ls_base->range.start;
                  out.result.push_back(code_lens);
                }
              }
            }
          } else {
            AddCodeLens("base", "base", &common, OffsetStartColumn(sym, 1),
                        ToUses(db, func.def->base),
                        false /*force_display*/);
          }

          break;
        }
        case SymbolKind::Var: {
          QueryVar& var = db->GetVar(sym);
          if (!var.def)
            continue;

          if (var.def->is_local() && !config->codeLensOnLocalVariables)
            continue;

          bool force_display = true;
          // Do not show 0 refs on macro with no uses, as it is most likely
          // a header guard.
          if (var.def->is_macro())
            force_display = false;

          AddCodeLens("ref", "refs", &common, OffsetStartColumn(sym, 0),
                      var.uses, force_display);
          break;
        }
        case SymbolKind::File:
        case SymbolKind::Invalid: {
          assert(false && "unexpected");
          break;
        }
      };
    }

    QueueManager::WriteStdout(IpcId::TextDocumentCodeLens, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentCodeLensHandler);
}  // namespace
