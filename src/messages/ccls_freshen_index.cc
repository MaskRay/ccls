#include "cache_manager.h"
#include "import_pipeline.h"
#include "match.h"
#include "message_handler.h"
#include "platform.h"
#include "project.h"
#include "queue_manager.h"
#include "timer.h"
#include "working_files.h"

#include <loguru.hpp>
#include <queue>
#include <unordered_set>

namespace {
MethodType kMethodType = "$ccls/freshenIndex";

struct In_CclsFreshenIndex : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct Params {
    bool dependencies = true;
    std::vector<std::string> whitelist;
    std::vector<std::string> blacklist;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(In_CclsFreshenIndex::Params,
                    dependencies,
                    whitelist,
                    blacklist);
MAKE_REFLECT_STRUCT(In_CclsFreshenIndex, params);
REGISTER_IN_MESSAGE(In_CclsFreshenIndex);

struct Handler_CclsFreshenIndex : BaseMessageHandler<In_CclsFreshenIndex> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_CclsFreshenIndex* request) override {
    LOG_S(INFO) << "Freshening " << project->entries.size() << " files";

    // TODO: think about this flow and test it more.
    GroupMatch matcher(request->params.whitelist, request->params.blacklist);

    // Unmark all files whose timestamp has changed.
    std::shared_ptr<ICacheManager> cache_manager = ICacheManager::Make();

    std::queue<const QueryFile*> q;
    // |need_index| stores every filename ever enqueued.
    std::unordered_set<std::string> need_index;
    // Reverse dependency graph.
    std::unordered_map<std::string, std::vector<std::string>> graph;
    // filename -> QueryFile mapping for files haven't enqueued.
    std::unordered_map<std::string, const QueryFile*> path_to_file;

    for (const auto& file : db->files)
      if (file.def) {
        if (matcher.IsMatch(file.def->path))
          q.push(&file);
        else
          path_to_file[file.def->path] = &file;
        for (const std::string& dependency : file.def->dependencies)
          graph[dependency].push_back(file.def->path);
      }

    while (!q.empty()) {
      const QueryFile* file = q.front();
      q.pop();
      need_index.insert(file->def->path);

      std::optional<int64_t> modification_timestamp =
          LastWriteTime(file->def->path);
      if (!modification_timestamp)
        continue;
      std::optional<int64_t> cached_modification =
          timestamp_manager->GetLastCachedModificationTime(cache_manager.get(),
                                                           file->def->path);
      if (modification_timestamp != cached_modification)
        file_consumer_shared->Reset(file->def->path);

      if (request->params.dependencies)
        for (const std::string& path : graph[file->def->path]) {
          auto it = path_to_file.find(path);
          if (it != path_to_file.end()) {
            q.push(it->second);
            path_to_file.erase(it);
          }
        }
    }

    Timer time;
    // Send index requests for every file.
    project->Index(QueueManager::instance(), working_files, lsRequestId());
    time.ResetAndPrint("[perf] Dispatched $ccls/freshenIndex index requests");
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CclsFreshenIndex);
}  // namespace
