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

Use OffsetStartColumn(Use use, int16_t offset) {
  use.range.start.column += offset;
  return use;
}

void AddCodeLens(const char* singular,
                 const char* plural,
                 CommonCodeLensParams* common,
                 Use use,
                 const std::vector<Use>& uses,
                 bool force_display) {
  TCodeLens code_lens;
  optional<lsRange> range = GetLsRange(common->working_file, use.range);
  if (!range)
    return;
  if (use.file == QueryFileId())
    return;
  code_lens.range = *range;
  code_lens.command = lsCommand<lsCodeLensCommandArguments>();
  code_lens.command->command = "cquery.showReferences";
  code_lens.command->arguments.uri = GetLsDocumentUri(common->db, use.file);
  code_lens.command->arguments.position = code_lens.range.start;

  // Add unique uses.
  std::unordered_set<lsLocation> unique_uses;
  for (Use use1 : uses) {
    optional<lsLocation> location =
        GetLsLocation(common->db, common->working_files, use1);
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
      Use use(sym.range, sym.id, sym.kind, sym.role, file->def->file);

      switch (sym.kind) {
        case SymbolKind::Type: {
          QueryType& type = db->GetType(sym);
          const QueryType::Def* def = type.AnyDef();
          if (!def || def->kind == lsSymbolKind::Namespace)
            continue;
          AddCodeLens("ref", "refs", &common, OffsetStartColumn(use, 0),
                      type.uses, true /*force_display*/);
          AddCodeLens("derived", "derived", &common, OffsetStartColumn(use, 1),
                      ToUses(db, type.derived), false /*force_display*/);
          AddCodeLens("var", "vars", &common, OffsetStartColumn(use, 2),
                      ToUses(db, type.instances), false /*force_display*/);
          break;
        }
        case SymbolKind::Func: {
          QueryFunc& func = db->GetFunc(sym);
          const QueryFunc::Def* def = func.AnyDef();
          if (!def)
            continue;

          int16_t offset = 0;

          // For functions, the outline will report a location that is using the
          // extent since that is better for outline. This tries to convert the
          // extent location to the spelling location.
          auto try_ensure_spelling = [&](Use use) {
            Maybe<Use> def = GetDefinitionSpellingOfSymbol(db, use);
            if (!def || def->range.start.line != use.range.start.line) {
              return use;
            }
            return *def;
          };

          std::vector<Use> base_callers =
              GetCallersForAllBaseFunctions(db, func);
          std::vector<Use> derived_callers =
              GetCallersForAllDerivedFunctions(db, func);
          if (base_callers.empty() && derived_callers.empty()) {
            Use loc = try_ensure_spelling(use);
            AddCodeLens("call", "calls", &common,
                        OffsetStartColumn(loc, offset++), func.uses,
                        true /*force_display*/);
          } else {
            Use loc = try_ensure_spelling(use);
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
                      OffsetStartColumn(use, offset++),
                      ToUses(db, func.derived), false /*force_display*/);

          // "Base"
          if (def->base.size() == 1) {
            Maybe<Use> base_loc = GetDefinitionSpellingOfSymbol(
                db, SymbolIdx{def->base[0], SymbolKind::Func});
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
            AddCodeLens("base", "base", &common, OffsetStartColumn(use, 1),
                        ToUses(db, def->base),
                        false /*force_display*/);
          }

          break;
        }
        case SymbolKind::Var: {
          QueryVar& var = db->GetVar(sym);
          const QueryVar::Def* def = var.AnyDef();
          if (!def || (def->is_local() && !config->codeLens.localVariables))
            continue;

          bool force_display = true;
          // Do not show 0 refs on macro with no uses, as it is most likely
          // a header guard.
          if (def->kind == lsSymbolKind::Macro)
            force_display = false;

          AddCodeLens("ref", "refs", &common, OffsetStartColumn(use, 0),
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
