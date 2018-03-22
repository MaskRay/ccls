#include "import_manager.h"
#include "import_pipeline.h"
#include "message_handler.h"
#include "queue_manager.h"

#include <loguru.hpp>

namespace {
MethodType kMethodType = "$cquery/wait";

struct In_CqueryWait : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
};
MAKE_REFLECT_EMPTY_STRUCT(In_CqueryWait);
REGISTER_IN_MESSAGE(In_CqueryWait);

struct Handler_CqueryWait : MessageHandler {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(std::unique_ptr<InMessage> request) override {
    // TODO: use status message system here, then run querydb as normal? Maybe
    // this cannot be a normal message, ie, it needs to be re-entrant.

    LOG_S(INFO) << "Waiting for idle";
    int idle_count = 0;
    while (true) {
      bool has_work = false;
      has_work |= import_pipeline_status->num_active_threads != 0;
      has_work |= QueueManager::instance()->HasWork();
      has_work |=
          QueryDb_ImportMain(config, db, import_manager, import_pipeline_status,
                             semantic_cache, working_files);
      if (!has_work)
        ++idle_count;
      else
        idle_count = 0;

      // There are race conditions between each of the three checks above,
      // so we retry a bunch of times to try to avoid any.
      if (idle_count > 10)
        break;
    }
    LOG_S(INFO) << "Done waiting for idle";
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CqueryWait);
}  // namespace
