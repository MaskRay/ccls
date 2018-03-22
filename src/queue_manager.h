#pragma once

#include "method.h"
#include "performance.h"
#include "query.h"
#include "threaded_queue.h"

#include <memory>

struct ICacheManager;
struct lsBaseOutMessage;

struct Stdout_Request {
  MethodType method;
  std::string content;
};

struct Index_Request {
  std::string path;
  // TODO: make |args| a string that is parsed lazily.
  std::vector<std::string> args;
  bool is_interactive;
  std::string contents;  // Preloaded contents.
  std::shared_ptr<ICacheManager> cache_manager;
  lsRequestId id;

  Index_Request(const std::string& path,
                const std::vector<std::string>& args,
                bool is_interactive,
                const std::string& contents,
                const std::shared_ptr<ICacheManager>& cache_manager,
                lsRequestId id = {});
};

struct Index_DoIdMap {
  std::unique_ptr<IndexFile> current;
  std::unique_ptr<IndexFile> previous;
  std::shared_ptr<ICacheManager> cache_manager;

  PerformanceImportFile perf;
  bool is_interactive = false;
  bool write_to_disk = false;
  bool load_previous = false;

  Index_DoIdMap(std::unique_ptr<IndexFile> current,
                const std::shared_ptr<ICacheManager>& cache_manager,
                PerformanceImportFile perf,
                bool is_interactive,
                bool write_to_disk);
};

struct Index_OnIdMapped {
  struct File {
    std::unique_ptr<IndexFile> file;
    std::unique_ptr<IdMap> ids;

    File(std::unique_ptr<IndexFile> file, std::unique_ptr<IdMap> ids);
  };

  std::unique_ptr<File> previous;
  std::unique_ptr<File> current;
  std::shared_ptr<ICacheManager> cache_manager;

  PerformanceImportFile perf;
  bool is_interactive;
  bool write_to_disk;

  Index_OnIdMapped(const std::shared_ptr<ICacheManager>& cache_manager,
                   PerformanceImportFile perf,
                   bool is_interactive,
                   bool write_to_disk);
};

struct Index_OnIndexed {
  IndexUpdate update;
  PerformanceImportFile perf;

  Index_OnIndexed(IndexUpdate&& update, PerformanceImportFile perf);
};

class QueueManager {
  static std::unique_ptr<QueueManager> instance_;

 public:
  static QueueManager* instance() { return instance_.get(); }
  static void Init(MultiQueueWaiter* querydb_waiter,
                   MultiQueueWaiter* indexer_waiter,
                   MultiQueueWaiter* stdout_waiter);
  static void WriteStdout(MethodType method, lsBaseOutMessage& response);

  bool HasWork();

  // Messages received by "stdout" thread.
  ThreadedQueue<Stdout_Request> for_stdout;

  // Runs on querydb thread.
  ThreadedQueue<std::unique_ptr<InMessage>> for_querydb;
  ThreadedQueue<Index_DoIdMap> do_id_map;

  // Runs on indexer threads.
  ThreadedQueue<Index_Request> index_request;
  ThreadedQueue<Index_DoIdMap> load_previous_index;
  ThreadedQueue<Index_OnIdMapped> on_id_mapped;

  // Shared by querydb and indexer.
  // TODO split on_indexed
  ThreadedQueue<Index_OnIndexed> on_indexed;

 private:
  explicit QueueManager(MultiQueueWaiter* querydb_waiter,
                        MultiQueueWaiter* indexer_waiter,
                        MultiQueueWaiter* stdout_waiter);
};
