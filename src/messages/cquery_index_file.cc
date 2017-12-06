#include "message_handler.h"
#include "platform.h"

struct CqueryIndexFileHandler : BaseMessageHandler<Ipc_CqueryIndexFile> {
  void Run(Ipc_CqueryIndexFile* request) override {
    queue->index_request.Enqueue(Index_Request(
        NormalizePath(request->params.path), request->params.args,
        request->params.is_interactive, request->params.contents));
  }
};
REGISTER_MESSAGE_HANDLER(CqueryIndexFileHandler);
