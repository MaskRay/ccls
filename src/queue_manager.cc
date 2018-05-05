#include "queue_manager.h"

#include "cache_manager.h"
#include "lsp.h"
#include "query.h"

#include <sstream>

Index_Request::Index_Request(
    const std::string& path,
    const std::vector<std::string>& args,
    bool is_interactive,
    const std::string& contents,
    lsRequestId id)
    : path(path),
      args(args),
      is_interactive(is_interactive),
      contents(contents),
      id(id) {}

Index_OnIndexed::Index_OnIndexed(IndexUpdate&& update,
                                 PerformanceImportFile perf)
    : update(std::move(update)), perf(perf) {}

std::unique_ptr<QueueManager> QueueManager::instance_;

// static
void QueueManager::Init(MultiQueueWaiter* querydb_waiter,
                        MultiQueueWaiter* indexer_waiter,
                        MultiQueueWaiter* stdout_waiter) {
  instance_ = std::unique_ptr<QueueManager>(
      new QueueManager(querydb_waiter, indexer_waiter, stdout_waiter));
}

// static
void QueueManager::WriteStdout(MethodType method, lsBaseOutMessage& response) {
  std::ostringstream sstream;
  response.Write(sstream);

  Stdout_Request out;
  out.content = sstream.str();
  out.method = method;
  instance()->for_stdout.PushBack(std::move(out));
}

QueueManager::QueueManager(MultiQueueWaiter* querydb_waiter,
                           MultiQueueWaiter* indexer_waiter,
                           MultiQueueWaiter* stdout_waiter)
    : for_stdout(stdout_waiter),
      for_querydb(querydb_waiter),
      on_indexed(querydb_waiter),
      index_request(indexer_waiter) {}
