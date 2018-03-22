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
    const std::shared_ptr<ICacheManager>& cache_manager,
    lsRequestId id)
    : path(path),
      args(args),
      is_interactive(is_interactive),
      contents(contents),
      cache_manager(cache_manager),
      id(id) {}

Index_DoIdMap::Index_DoIdMap(
    std::unique_ptr<IndexFile> current,
    const std::shared_ptr<ICacheManager>& cache_manager,
    PerformanceImportFile perf,
    bool is_interactive,
    bool write_to_disk)
    : current(std::move(current)),
      cache_manager(cache_manager),
      perf(perf),
      is_interactive(is_interactive),
      write_to_disk(write_to_disk) {
  assert(this->current);
}

Index_OnIdMapped::File::File(std::unique_ptr<IndexFile> file,
                             std::unique_ptr<IdMap> ids)
    : file(std::move(file)), ids(std::move(ids)) {}

Index_OnIdMapped::Index_OnIdMapped(
    const std::shared_ptr<ICacheManager>& cache_manager,
    PerformanceImportFile perf,
    bool is_interactive,
    bool write_to_disk)
    : cache_manager(cache_manager),
      perf(perf),
      is_interactive(is_interactive),
      write_to_disk(write_to_disk) {}

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
      do_id_map(querydb_waiter),
      index_request(indexer_waiter),
      load_previous_index(indexer_waiter),
      on_id_mapped(indexer_waiter),
      // TODO on_indexed is shared by "querydb" and "indexer"
      on_indexed(querydb_waiter, indexer_waiter) {}

bool QueueManager::HasWork() {
  return !index_request.IsEmpty() || !do_id_map.IsEmpty() ||
         !load_previous_index.IsEmpty() || !on_id_mapped.IsEmpty() ||
         !on_indexed.IsEmpty();
}
