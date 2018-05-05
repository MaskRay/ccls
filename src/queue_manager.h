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
  lsRequestId id;

  Index_Request(const std::string& path,
                const std::vector<std::string>& args,
                bool is_interactive,
                const std::string& contents,
                lsRequestId id = {});
};

struct Index_OnIdMapped {
  std::shared_ptr<ICacheManager> cache_manager;
  std::unique_ptr<IndexFile> previous;
  std::unique_ptr<IndexFile> current;

  PerformanceImportFile perf;
  bool is_interactive;
  bool write_to_disk;

  Index_OnIdMapped(const std::shared_ptr<ICacheManager>& cache_manager,
                   std::unique_ptr<IndexFile> previous,
                   std::unique_ptr<IndexFile> current,
                   PerformanceImportFile perf,
                   bool is_interactive,
                   bool write_to_disk)
      : cache_manager(cache_manager),
        previous(std::move(previous)),
        current(std::move(current)),
        perf(perf),
        is_interactive(is_interactive),
        write_to_disk(write_to_disk) {}
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

  // Messages received by "stdout" thread.
  ThreadedQueue<Stdout_Request> for_stdout;

  // Runs on querydb thread.
  ThreadedQueue<std::unique_ptr<InMessage>> for_querydb;
  ThreadedQueue<Index_OnIndexed> on_indexed;

  // Runs on indexer threads.
  ThreadedQueue<Index_Request> index_request;

 private:
  explicit QueueManager(MultiQueueWaiter* querydb_waiter,
                        MultiQueueWaiter* indexer_waiter,
                        MultiQueueWaiter* stdout_waiter);
};
