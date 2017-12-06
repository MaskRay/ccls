#include "message_handler.h"
#include "query_utils.h"

struct TextDocumentCodeLensHandler
    : BaseMessageHandler<Ipc_TextDocumentCodeLens> {
  void Run(Ipc_TextDocumentCodeLens* request) override {
    Out_TextDocumentCodeLens out;
    out.id = request->id;

    lsDocumentUri file_as_uri = request->params.textDocument.uri;
    std::string path = file_as_uri.GetPath();

    clang_complete->NotifyView(path);

    QueryFile* file;
    if (!FindFileOrFail(db, request->id,
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
          AddCodeLens("ref", "refs", &common, ref.loc.OffsetStartColumn(0),
                      type.uses, type.def->definition_spelling,
                      true /*force_display*/);
          AddCodeLens("derived", "derived", &common,
                      ref.loc.OffsetStartColumn(1),
                      ToQueryLocation(db, type.derived), nullopt,
                      false /*force_display*/);
          AddCodeLens("var", "vars", &common, ref.loc.OffsetStartColumn(2),
                      ToQueryLocation(db, type.instances), nullopt,
                      false /*force_display*/);
          break;
        }
        case SymbolKind::Func: {
          QueryFunc& func = db->funcs[symbol.idx];
          if (!func.def)
            continue;

          int16_t offset = 0;

          std::vector<QueryFuncRef> base_callers =
              GetCallersForAllBaseFunctions(db, func);
          std::vector<QueryFuncRef> derived_callers =
              GetCallersForAllDerivedFunctions(db, func);
          if (base_callers.empty() && derived_callers.empty()) {
            AddCodeLens("call", "calls", &common,
                        ref.loc.OffsetStartColumn(offset++),
                        ToQueryLocation(db, func.callers), nullopt,
                        true /*force_display*/);
          } else {
            AddCodeLens("direct call", "direct calls", &common,
                        ref.loc.OffsetStartColumn(offset++),
                        ToQueryLocation(db, func.callers), nullopt,
                        false /*force_display*/);
            if (!base_callers.empty())
              AddCodeLens("base call", "base calls", &common,
                          ref.loc.OffsetStartColumn(offset++),
                          ToQueryLocation(db, base_callers), nullopt,
                          false /*force_display*/);
            if (!derived_callers.empty())
              AddCodeLens("derived call", "derived calls", &common,
                          ref.loc.OffsetStartColumn(offset++),
                          ToQueryLocation(db, derived_callers), nullopt,
                          false /*force_display*/);
          }

          AddCodeLens("derived", "derived", &common,
                      ref.loc.OffsetStartColumn(offset++),
                      ToQueryLocation(db, func.derived), nullopt,
                      false /*force_display*/);

          // "Base"
          optional<QueryLocation> base_loc =
              GetBaseDefinitionOrDeclarationSpelling(db, func);
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

          break;
        }
        case SymbolKind::Var: {
          QueryVar& var = db->vars[symbol.idx];
          if (!var.def)
            continue;

          if (var.def->is_local && !config->codeLensOnLocalVariables)
            continue;

          bool force_display = true;
          // Do not show 0 refs on macro with no uses, as it is most likely
          // a header guard.
          if (var.def->is_macro)
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

    IpcManager::WriteStdout(IpcId::TextDocumentCodeLens, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentCodeLensHandler);
