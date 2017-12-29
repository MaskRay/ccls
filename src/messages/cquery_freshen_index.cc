#include "cache_manager.h"
#include "message_handler.h"
#include "platform.h"
#include "project.h"
#include "queue_manager.h"
#include "timestamp_manager.h"
#include "working_files.h"

#include <loguru.hpp>

namespace {
struct Ipc_CqueryFreshenIndex : public IpcMessage<Ipc_CqueryFreshenIndex> {
  const static IpcId kIpcId = IpcId::CqueryFreshenIndex;
  lsRequestId id;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryFreshenIndex, id);
REGISTER_IPC_MESSAGE(Ipc_CqueryFreshenIndex);

struct CqueryFreshenIndexHandler : MessageHandler {
  IpcId GetId() const override { return IpcId::CqueryFreshenIndex; }

  void Run(std::unique_ptr<BaseIpcMessage> request) override {
    LOG_S(INFO) << "Freshening " << project->entries.size() << " files";

    // TODO: think about this flow and test it more.

    // Unmark all files whose timestamp has changed.
    std::unique_ptr<ICacheManager> cache_manager = ICacheManager::Make(config);
    for (const auto& file : db->files) {
      if (!file.def)
        continue;

      optional<int64_t> modification_timestamp =
          GetLastModificationTime(file.def->path);
      if (!modification_timestamp)
        continue;

      optional<int64_t> cached_modification =
          timestamp_manager->GetLastCachedModificationTime(cache_manager.get(),
                                                           file.def->path);
      if (modification_timestamp != cached_modification)
        file_consumer_shared->Reset(file.def->path);
    }

    auto* queue = QueueManager::instance();

    // Send index requests for every file.
    project->ForAllFilteredFiles(config, [&](int i,
                                             const Project::Entry& entry) {
      LOG_S(INFO) << "[" << i << "/" << (project->entries.size() - 1)
                  << "] Dispatching index request for file " << entry.filename;
      bool is_interactive =
          working_files->GetFileByFilename(entry.filename) != nullptr;
      queue->index_request.Enqueue(
          Index_Request(entry.filename, entry.args, is_interactive, nullopt));
    });
  }
};
REGISTER_MESSAGE_HANDLER(CqueryFreshenIndexHandler);
}  // namespace