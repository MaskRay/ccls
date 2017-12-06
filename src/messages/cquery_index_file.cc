#include "message_handler.h"
#include "platform.h"

namespace {
struct Ipc_CqueryIndexFile : public IpcMessage<Ipc_CqueryIndexFile> {
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
    queue->index_request.Enqueue(Index_Request(
        NormalizePath(request->params.path), request->params.args,
        request->params.is_interactive, request->params.contents));
  }
};
REGISTER_MESSAGE_HANDLER(CqueryIndexFileHandler);
}  // namespace