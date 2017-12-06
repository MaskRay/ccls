#include "message_handler.h"
#include "query_utils.h"

namespace {
void PushBack(NonElidedVector<lsLocation>* result,
              optional<lsLocation> location) {
  if (location)
    result->push_back(*location);
}
}  // namespace

struct Ipc_TextDocumentDefinition
    : public IpcMessage<Ipc_TextDocumentDefinition> {
  const static IpcId kIpcId = IpcId::TextDocumentDefinition;

  lsRequestId id;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDefinition, id, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDefinition);

struct Out_TextDocumentDefinition
    : public lsOutMessage<Out_TextDocumentDefinition> {
  lsRequestId id;
  NonElidedVector<lsLocation> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentDefinition, jsonrpc, id, result);

struct TextDocumentDefinitionHandler
    : BaseMessageHandler<Ipc_TextDocumentDefinition> {
  void Run(Ipc_TextDocumentDefinition* request) override {
    QueryFileId file_id;
    QueryFile* file;
    if (!FindFileOrFail(db, request->id,
                        request->params.textDocument.uri.GetPath(), &file,
                        &file_id)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_TextDocumentDefinition out;
    out.id = request->id;

    int target_line = request->params.position.line + 1;
    int target_column = request->params.position.character + 1;

    for (const SymbolRef& ref :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      // Found symbol. Return definition.

      // Special cases which are handled:
      //  - symbol has declaration but no definition (ie, pure virtual)
      //  - start at spelling but end at extent for better mouse tooltip
      //  - goto declaration while in definition of recursive type

      optional<QueryLocation> def_loc =
          GetDefinitionSpellingOfSymbol(db, ref.idx);

      // We use spelling start and extent end because this causes vscode to
      // highlight the entire definition when previewing / hoving with the
      // mouse.
      optional<QueryLocation> def_extent =
          GetDefinitionExtentOfSymbol(db, ref.idx);
      if (def_loc && def_extent)
        def_loc->range.end = def_extent->range.end;

      // If the cursor is currently at or in the definition we should goto
      // the declaration if possible. We also want to use declarations if
      // we're pointing to, ie, a pure virtual function which has no
      // definition.
      if (!def_loc || (def_loc->path == file_id &&
                       def_loc->range.Contains(target_line, target_column))) {
        // Goto declaration.

        std::vector<QueryLocation> declarations =
            GetDeclarationsOfSymbolForGotoDefinition(db, ref.idx);
        for (auto declaration : declarations) {
          optional<lsLocation> ls_declaration =
              GetLsLocation(db, working_files, declaration);
          if (ls_declaration)
            out.result.push_back(*ls_declaration);
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

    IpcManager::WriteStdout(IpcId::TextDocumentDefinition, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentDefinitionHandler);
