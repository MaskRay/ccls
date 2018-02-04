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

void AddCodeLens(const char* singular,
                 const char* plural,
                 CommonCodeLensParams* common,
                 QueryLocation loc,
                 const std::vector<QueryLocation>& uses,
                 optional<QueryLocation> excluded,
                 bool force_display) {
  TCodeLens code_lens;
  optional<lsRange> range = GetLsRange(common->working_file, loc.range);
  if (!range)
    return;
  code_lens.range = *range;
  code_lens.command = lsCommand<lsCodeLensCommandArguments>();
  code_lens.command->command = "cquery.showReferences";
  code_lens.command->arguments.uri = GetLsDocumentUri(common->db, loc.path);
  code_lens.command->arguments.position = code_lens.range.start;

  // Add unique uses.
  std::unordered_set<lsLocation> unique_uses;
  for (const QueryLocation& use : uses) {
    if (excluded == use)
      continue;
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

    for (SymbolRef ref : file->def->outline) {
      // NOTE: We OffsetColumn so that the code lens always show up in a
      // predictable order. Otherwise, the client may randomize it.

      SymbolIdx symbol = ref.idx;
      switch (symbol.kind) {
        case SymbolKind::Type: {
          QueryType& type = db->types[symbol.idx];
          if (!type.def)
            continue;
          if (type.def->kind == ClangSymbolKind::Namespace)
            continue;
          AddCodeLens("ref", "refs", &common, ref.loc.OffsetStartColumn(0),
                      type.uses, type.def->definition_spelling,
                      true /*force_display*/);
          AddCodeLens("derived", "derived", &common,
                      ref.loc.OffsetStartColumn(1),
                      ToQueryLocation(db, &type.derived), nullopt,
                      false /*force_display*/);
          AddCodeLens("var", "vars", &common, ref.loc.OffsetStartColumn(2),
                      ToQueryLocation(db, &type.instances), nullopt,
                      false /*force_display*/);
          break;
        }
        case SymbolKind::Func: {
          QueryFunc& func = db->funcs[symbol.idx];
          if (!func.def)
            continue;

          int16_t offset = 0;

          // For functions, the outline will report a location that is using the
          // extent since that is better for outline. This tries to convert the
          // extent location to the spelling location.
          auto try_ensure_spelling = [&](SymbolRef sym) {
            optional<QueryLocation> def =
                GetDefinitionSpellingOfSymbol(db, sym.idx);
            if (!def || def->path != sym.loc.path ||
                def->range.start.line != sym.loc.range.start.line) {
              return sym.loc;
            }
            return *def;
          };

          std::vector<QueryFuncRef> base_callers =
              GetCallersForAllBaseFunctions(db, func);
          std::vector<QueryFuncRef> derived_callers =
              GetCallersForAllDerivedFunctions(db, func);
          if (base_callers.empty() && derived_callers.empty()) {
            QueryLocation loc = try_ensure_spelling(ref);
            AddCodeLens("call", "calls", &common,
                        loc.OffsetStartColumn(offset++),
                        ToQueryLocation(db, func.callers), nullopt,
                        true /*force_display*/);
          } else {
            QueryLocation loc = try_ensure_spelling(ref);
            AddCodeLens("direct call", "direct calls", &common,
                        loc.OffsetStartColumn(offset++),
                        ToQueryLocation(db, func.callers), nullopt,
                        false /*force_display*/);
            if (!base_callers.empty())
              AddCodeLens("base call", "base calls", &common,
                          loc.OffsetStartColumn(offset++),
                          ToQueryLocation(db, base_callers), nullopt,
                          false /*force_display*/);
            if (!derived_callers.empty())
              AddCodeLens("derived call", "derived calls", &common,
                          loc.OffsetStartColumn(offset++),
                          ToQueryLocation(db, derived_callers), nullopt,
                          false /*force_display*/);
          }

          AddCodeLens("derived", "derived", &common,
                      ref.loc.OffsetStartColumn(offset++),
                      ToQueryLocation(db, &func.derived), nullopt,
                      false /*force_display*/);

          // "Base"
          if (func.def->base.size() == 1) {
            // FIXME WithGen
            optional<QueryLocation> base_loc =
                GetDefinitionSpellingOfSymbol(db, func.def->base[0].value);
            if (base_loc) {
              optional<lsLocation> ls_base =
                  GetLsLocation(db, working_files, *base_loc);
              if (ls_base) {
                optional<lsRange> range =
                    GetLsRange(common.working_file, ref.loc.range);
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
            AddCodeLens("base", "base", &common, ref.loc.OffsetStartColumn(1),
                        ToQueryLocation(db, &func.def->base), nullopt,
                        false /*force_display*/);
          }

          break;
        }
        case SymbolKind::Var: {
          QueryVar& var = db->vars[symbol.idx];
          if (!var.def)
            continue;

          if (var.def->is_local() && !config->codeLensOnLocalVariables)
            continue;

          bool force_display = true;
          // Do not show 0 refs on macro with no uses, as it is most likely
          // a header guard.
          if (var.def->is_macro())
            force_display = false;

          AddCodeLens("ref", "refs", &common, ref.loc.OffsetStartColumn(0),
                      var.uses, var.def->definition_spelling, force_display);
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
