#include "cache_manager.h"
#include "message_handler.h"
#include "platform.h"
#include "queue_manager.h"

#include <loguru/loguru.hpp>

namespace {
MethodType kMethodType = "$cquery/indexFile";

struct In_CqueryIndexFile : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct Params {
    std::string path;
    std::vector<std::string> args;
    bool is_interactive = false;
    std::string contents;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(In_CqueryIndexFile::Params,
                    path,
                    args,
                    is_interactive,
                    contents);
MAKE_REFLECT_STRUCT(In_CqueryIndexFile, params);
REGISTER_IN_MESSAGE(In_CqueryIndexFile);

struct Handler_CqueryIndexFile : BaseMessageHandler<In_CqueryIndexFile> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_CqueryIndexFile* request) override {
    LOG_S(INFO) << "Indexing file " << request->params.path;
    QueueManager::instance()->index_request.PushBack(
        Index_Request(NormalizePath(request->params.path), request->params.args,
                      request->params.is_interactive, request->params.contents,
                      ICacheManager::Make(config)));
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CqueryIndexFile);
}  // namespace
