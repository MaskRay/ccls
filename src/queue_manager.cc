#include "queue_manager.h"

#include "language_server_api.h"
#include "query.h"

#include <sstream>

Index_Request::Index_Request(const std::string& path,
                             const std::vector<std::string>& args,
                             bool is_interactive,
                             const std::string& contents)
    : path(path),
      args(args),
      is_interactive(is_interactive),
      contents(contents) {}

Index_DoIdMap::Index_DoIdMap(std::unique_ptr<IndexFile> current,
                             PerformanceImportFile perf,
                             bool is_interactive,
                             bool write_to_disk)
    : current(std::move(current)),
      perf(perf),
      is_interactive(is_interactive),
      write_to_disk(write_to_disk) {
  assert(this->current);
}

Index_OnIdMapped::File::File(std::unique_ptr<IndexFile> file,
                             std::unique_ptr<IdMap> ids)
    : file(std::move(file)), ids(std::move(ids)) {}

Index_OnIdMapped::Index_OnIdMapped(PerformanceImportFile perf,
                                   bool is_interactive,
                                   bool write_to_disk)
    : perf(perf),
      is_interactive(is_interactive),
      write_to_disk(write_to_disk) {}

Index_OnIndexed::Index_OnIndexed(IndexUpdate& update,
                                 PerformanceImportFile perf)
    : update(update), perf(perf) {}

QueueManager* QueueManager::instance_ = nullptr;

// static
QueueManager* QueueManager::instance() {
  return instance_;
}

// static
void QueueManager::CreateInstance(MultiQueueWaiter* querydb_waiter,
                                  MultiQueueWaiter* indexer_waiter,
                                  MultiQueueWaiter* stdout_waiter) {
  if (instance_)
    delete instance_;
  instance_ = new QueueManager(querydb_waiter, indexer_waiter, stdout_waiter);
}

// static
void QueueManager::WriteStdout(IpcId id, lsBaseOutMessage& response) {
  std::ostringstream sstream;
  response.Write(sstream);

  Stdout_Request out;
  out.content = sstream.str();
  out.id = id;
  instance()->for_stdout.Enqueue(std::move(out));
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
