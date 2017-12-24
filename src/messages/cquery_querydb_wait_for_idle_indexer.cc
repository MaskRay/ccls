#include "entry_points.h"
#include "message_handler.h"

#include <loguru.hpp>

namespace {
struct Ipc_CqueryQueryDbWaitForIdleIndexer
    : public IpcMessage<Ipc_CqueryQueryDbWaitForIdleIndexer> {
  static constexpr IpcId kIpcId = IpcId::CqueryQueryDbWaitForIdleIndexer;
};
MAKE_REFLECT_EMPTY_STRUCT(Ipc_CqueryQueryDbWaitForIdleIndexer);
REGISTER_IPC_MESSAGE(Ipc_CqueryQueryDbWaitForIdleIndexer);

struct CqueryQueryDbWaitForIdleIndexerHandler : MessageHandler {
  IpcId GetId() const override {
    return IpcId::CqueryQueryDbWaitForIdleIndexer;
  }
  void Run(std::unique_ptr<BaseIpcMessage> request) override {
    LOG_S(INFO) << "Waiting for idle";
    int idle_count = 0;
    while (true) {
      bool has_work = false;
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
REGISTER_MESSAGE_HANDLER(CqueryQueryDbWaitForIdleIndexerHandler);
}  // namespace