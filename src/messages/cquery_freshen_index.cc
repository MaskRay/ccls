#include "message_handler.h"
#include "platform.h"

#include <loguru.hpp>

struct CqueryFreshenIndexHandler : MessageHandler {
  IpcId GetId() const override { return IpcId::CqueryFreshenIndex; }

  void Run(std::unique_ptr<BaseIpcMessage> request) {
    LOG_S(INFO) << "Freshening " << project->entries.size() << " files";

    // TODO: think about this flow and test it more.

    // Unmark all files whose timestamp has changed.
    CacheLoader cache_loader(config);
    for (const auto& file : db->files) {
      if (!file.def)
        continue;

      optional<int64_t> modification_timestamp =
          GetLastModificationTime(file.def->path);
      if (!modification_timestamp)
        continue;

      optional<int64_t> cached_modification =
          timestamp_manager->GetLastCachedModificationTime(&cache_loader,
                                                           file.def->path);
      if (modification_timestamp != cached_modification)
        file_consumer_shared->Reset(file.def->path);
    }

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
