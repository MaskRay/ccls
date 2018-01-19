#include "message_handler.h"
#include "platform.h"
#include "queue_manager.h"

#include <loguru/loguru.hpp>

namespace {
struct Ipc_CqueryIndexFile : public NotificationMessage<Ipc_CqueryIndexFile> {
  static constexpr IpcId kIpcId = IpcId::CqueryIndexFile;
  struct Params {
    std::string path;
    std::vector<std::string> args;
    bool is_interactive = false;
    std::string contents;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryIndexFile::Params,
                    path,
                    args,
                    is_interactive,
                    contents);
MAKE_REFLECT_STRUCT(Ipc_CqueryIndexFile, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryIndexFile);

struct CqueryIndexFileHandler : BaseMessageHandler<Ipc_CqueryIndexFile> {
  void Run(Ipc_CqueryIndexFile* request) override {
    LOG_S(INFO) << "Indexing file " << request->params.path;
    QueueManager::instance()->index_request.Enqueue(Index_Request(
        NormalizePath(request->params.path), request->params.args,
        request->params.is_interactive, request->params.contents));
  }
};
REGISTER_MESSAGE_HANDLER(CqueryIndexFileHandler);
}  // namespace
