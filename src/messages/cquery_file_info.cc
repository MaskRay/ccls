#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
MethodType kMethodType = "$cquery/fileInfo";

struct lsDocumentSymbolParams {
  lsTextDocumentIdentifier textDocument;
};
MAKE_REFLECT_STRUCT(lsDocumentSymbolParams, textDocument);

struct In_CqueryFileInfo : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsDocumentSymbolParams params;
};
MAKE_REFLECT_STRUCT(In_CqueryFileInfo, id, params);
REGISTER_IN_MESSAGE(In_CqueryFileInfo);

struct Out_CqueryFileInfo : public lsOutMessage<Out_CqueryFileInfo> {
  lsRequestId id;
  QueryFile::Def result;
};
MAKE_REFLECT_STRUCT(Out_CqueryFileInfo, jsonrpc, id, result);

struct Handler_CqueryFileInfo : BaseMessageHandler<In_CqueryFileInfo> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_CqueryFileInfo* request) override {
    QueryFile* file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    Out_CqueryFileInfo out;
    out.id = request->id;
    // Expose some fields of |QueryFile::Def|.
    out.result.path = file->def->path;
    out.result.args = file->def->args;
    out.result.language = file->def->language;
    out.result.includes = file->def->includes;
    out.result.inactive_regions = file->def->inactive_regions;
    QueueManager::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CqueryFileInfo);
}  // namespace
