#include "import_manager.h"
#include "import_pipeline.h"
#include "message_handler.h"
#include "queue_manager.h"

#include <loguru.hpp>

namespace {
struct Ipc_CqueryWait : public IpcMessage<Ipc_CqueryWait> {
  static constexpr IpcId kIpcId = IpcId::CqueryWait;
};
MAKE_REFLECT_EMPTY_STRUCT(Ipc_CqueryWait);
REGISTER_IPC_MESSAGE(Ipc_CqueryWait);

struct CqueryWaitHandler : MessageHandler {
  IpcId GetId() const override { return IpcId::CqueryWait; }
  void Run(std::unique_ptr<BaseIpcMessage> request) override {
    // TODO: use status message system here, then run querydb as normal? Maybe
    // this cannot be a normal message, ie, it needs to be re-entrant.

    LOG_S(INFO) << "Waiting for idle";
    int idle_count = 0;
    while (true) {
      bool has_work = false;
      has_work |= import_pipeline_status->num_active_threads != 0;
      has_work |= import_manager->HasActiveQuerydbImports();
      has_work |= QueueManager::instance()->HasWork();
      has_work |= QueryDb_ImportMain(config, db, import_manager, semantic_cache,
                                     working_files);
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
REGISTER_MESSAGE_HANDLER(CqueryWaitHandler);
}  // namespace
