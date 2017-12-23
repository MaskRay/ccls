#include "message_handler.h"
#include "query_utils.h"

namespace {

std::string GetHoverForSymbol(QueryDatabase* db, const SymbolIdx& symbol) {
  switch (symbol.kind) {
    case SymbolKind::Type: {
      QueryType& type = db->types[symbol.idx];
      if (type.def) {
        if (type.def->hover)
          return *type.def->hover;
        return type.def->detailed_name;
      }
      break;
    }
    case SymbolKind::Func: {
      QueryFunc& func = db->funcs[symbol.idx];
      if (func.def) {
        if (func.def->hover)
          return *func.def->hover;
        return func.def->detailed_name;
      }
      break;
    }
    case SymbolKind::Var: {
      QueryVar& var = db->vars[symbol.idx];
      if (var.def) {
        if (var.def->hover)
          return *var.def->hover;
        return var.def->detailed_name;
      }
      break;
    }
    case SymbolKind::File:
    case SymbolKind::Invalid: {
      assert(false && "unexpected");
      break;
    }
  }
  return "";
}

struct Ipc_TextDocumentHover : public IpcMessage<Ipc_TextDocumentHover> {
  const static IpcId kIpcId = IpcId::TextDocumentHover;

  lsRequestId id;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentHover, id, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentHover);

struct Out_TextDocumentHover : public lsOutMessage<Out_TextDocumentHover> {
  struct Result {
    lsMarkedString contents;
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
    if (!FindFileOrFail(db, request->id,
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

      std::string hover = GetHoverForSymbol(db, ref.idx);
      if (!hover.empty()) {
        out.result = Out_TextDocumentHover::Result();
        out.result->contents.value = hover;
        out.result->contents.language = file->def->language;
        out.result->range = *ls_range;
        break;
      }
    }

    IpcManager::WriteStdout(IpcId::TextDocumentHover, out);
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentHoverHandler);
}  // namespace
