#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

namespace {
struct lsDocumentSymbolParams {
  lsTextDocumentIdentifier textDocument;
};
MAKE_REFLECT_STRUCT(lsDocumentSymbolParams, textDocument);

struct Ipc_CqueryFileInfo : public RequestMessage<Ipc_CqueryFileInfo> {
  const static IpcId kIpcId = IpcId::CqueryFileInfo;
  lsDocumentSymbolParams params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryFileInfo, id, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryFileInfo);

struct Out_CqueryFileInfo : public lsOutMessage<Out_CqueryFileInfo> {
  lsRequestId id;
  QueryFile::Def result;
};
MAKE_REFLECT_STRUCT(Out_CqueryFileInfo, jsonrpc, id, result);

struct CqueryFileInfoHandler : BaseMessageHandler<Ipc_CqueryFileInfo> {
  void Run(Ipc_CqueryFileInfo* request) override {
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
    QueueManager::WriteStdout(IpcId::CqueryFileInfo, out);
  }
};
REGISTER_MESSAGE_HANDLER(CqueryFileInfoHandler);
}  // namespace
