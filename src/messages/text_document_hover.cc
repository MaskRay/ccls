#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {

std::pair<optional<std::string>, std::string> GetCommentsAndHover(
    QueryDatabase* db,
    const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      QueryType& type = db->types[symbol.idx];
      if (type.def)
        return {type.def->comments,
                type.def->hover.value_or(type.def->detailed_name)};
      break;
    }
    case SymbolKind::Func: {
      QueryFunc& func = db->funcs[symbol.idx];
      if (func.def)
        return {func.def->comments,
                func.def->hover.value_or(func.def->detailed_name)};
      break;
    }
    case SymbolKind::Var: {
      QueryVar& var = db->vars[symbol.idx];
      if (var.def)
        return {var.def->comments,
                var.def->hover.value_or(var.def->detailed_name)};
      break;
    }
    case SymbolKind::File:
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return {nullopt, ""};
}

struct Ipc_TextDocumentHover : public RequestMessage<Ipc_TextDocumentHover> {
  const static IpcId kIpcId = IpcId::TextDocumentHover;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentHover, id, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentHover);

struct Out_TextDocumentHover : public lsOutMessage<Out_TextDocumentHover> {
  struct Result {
    std::vector<lsMarkedString> contents;
    optional<lsRange> range;
  };

  lsRequestId id;
  optional<Result> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentHover::Result, contents, range);
void Reflect(Writer& visitor, Out_TextDocumentHover& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(jsonrpc);
  REFLECT_MEMBER(id);
  if (value.result)
    REFLECT_MEMBER(result);
  else {
    // Empty optional<> is elided by the default serializer, we need to write
    // |null| to be compliant with the LSP.
    visitor.Key("result");
    visitor.Null();
  }
  REFLECT_MEMBER_END();
}

struct TextDocumentHoverHandler : BaseMessageHandler<Ipc_TextDocumentHover> {
  void Run(Ipc_TextDocumentHover* request) override {
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    WorkingFile* working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_TextDocumentHover out;
    out.id = request->id;

    for (const SymbolRef& ref :
         FindSymbolsAtLocation(working_file, file, request->params.position)) {
      // Found symbol. Return hover.
      optional<lsRange> ls_range = GetLsRange(
          working_files->GetFileByFilename(file->def->path), ref.loc.range);
      if (!ls_range)
        continue;

      std::pair<optional<std::string>, std::string> comments_hover =
          GetCommentsAndHover(db, ref.idx);
      if (comments_hover.first || comments_hover.second.size()) {
        out.result = Out_TextDocumentHover::Result();
        if (comments_hover.first) {
          out.result->contents.emplace_back(*comments_hover.first);
        }
        if (comments_hover.second.size()) {
          out.result->contents.emplace_back(
              lsMarkedString1{file->def->language, comments_hover.second});
        }
        out.result->range = *ls_range;
        break;
      }
    }

    QueueManager::WriteStdout(IpcId::TextDocumentHover, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentHoverHandler);
}  // namespace
