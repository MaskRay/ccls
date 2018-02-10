#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
void PushBack(std::vector<lsLocation>* result, optional<lsLocation> location) {
  if (location)
    result->push_back(*location);
}

struct Ipc_TextDocumentDefinition
    : public RequestMessage<Ipc_TextDocumentDefinition> {
  const static IpcId kIpcId = IpcId::TextDocumentDefinition;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDefinition, id, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDefinition);

struct Out_TextDocumentDefinition
    : public lsOutMessage<Out_TextDocumentDefinition> {
  lsRequestId id;
  std::vector<lsLocation> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentDefinition, jsonrpc, id, result);

std::vector<Use> GetGotoDefinitionTargets(QueryDatabase* db,
                                          SymbolRef sym) {
  switch (sym.kind) {
    // Returns GetDeclarationsOfSymbolForGotoDefinition and
    // variable type definition.
    case SymbolKind::Var: {
      std::vector<Use> ret =
          GetDeclarationsOfSymbolForGotoDefinition(db, sym);
      QueryVar& var = db->GetVar(sym);
      if (var.def && var.def->variable_type) {
        std::vector<Use> types = GetDeclarationsOfSymbolForGotoDefinition(
            db, SymbolRef(Range(), Id<void>(var.def->variable_type->id),
                          SymbolKind::Type, SymbolRole::None));
        ret.insert(ret.end(), types.begin(), types.end());
      }
      return ret;
    }
    default:
      return GetDeclarationsOfSymbolForGotoDefinition(db, sym);
  }
}

struct TextDocumentDefinitionHandler
    : BaseMessageHandler<Ipc_TextDocumentDefinition> {
  void Run(Ipc_TextDocumentDefinition* request) override {
    QueryFileId file_id;
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file,
                        &file_id)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_TextDocumentDefinition out;
    out.id = request->id;

    int target_line = request->params.position.line;
    int target_column = request->params.position.character;

    for (SymbolRef sym :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      // Found symbol. Return definition.

      // Special cases which are handled:
      //  - symbol has declaration but no definition (ie, pure virtual)
      //  - start at spelling but end at extent for better mouse tooltip
      //  - goto declaration while in definition of recursive type

      Maybe<Reference> def_loc = GetDefinitionSpellingOfSymbol(db, sym);

      // We use spelling start and extent end because this causes vscode to
      // highlight the entire definition when previewing / hoving with the
      // mouse.
      Maybe<Reference> def_extent = GetDefinitionExtentOfSymbol(db, sym);
      if (def_loc && def_extent)
        def_loc->range.end = def_extent->range.end;

      // If the cursor is currently at or in the definition we should goto
      // the declaration if possible. We also want to use declarations if
      // we're pointing to, ie, a pure virtual function which has no
      // definition.
      if (!def_loc || (db->GetFileId(*def_loc) == file_id &&
                       def_loc->range.Contains(target_line, target_column))) {
        // Goto declaration.

        std::vector<Use> targets = GetGotoDefinitionTargets(db, sym);
        for (Use target : targets) {
          optional<lsLocation> ls_target =
              GetLsLocation(db, working_files, target);
          if (ls_target)
            out.result.push_back(*ls_target);
        }
        // We found some declarations. Break so we don't add the definition
        // location.
        if (!out.result.empty())
          break;
      }

      if (def_loc) {
        PushBack(&out.result, GetLsLocation(db, working_files, *def_loc));
      }

      if (!out.result.empty())
        break;
    }

    // No symbols - check for includes.
    if (out.result.empty()) {
      for (const IndexInclude& include : file->def->includes) {
        if (include.line == target_line) {
          lsLocation result;
          result.uri = lsDocumentUri::FromPath(include.resolved_path);
          out.result.push_back(result);
          break;
        }
      }
    }

    QueueManager::WriteStdout(IpcId::TextDocumentDefinition, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentDefinitionHandler);
}  // namespace
