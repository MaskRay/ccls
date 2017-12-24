#pragma once

#include "ipc.h"
#include "performance.h"
#include "query.h"
#include "threaded_queue.h"

#include <memory>

struct lsBaseOutMessage;

struct Stdout_Request {
  IpcId id;
  std::string content;
};

struct Index_Request {
  std::string path;
  // TODO: make |args| a string that is parsed lazily.
  std::vector<std::string> args;
  bool is_interactive;
  optional<std::string> contents;  // Preloaded contents. Useful for tests.

  Index_Request(const std::string& path,
                const std::vector<std::string>& args,
                bool is_interactive,
                optional<std::string> contents);
};

struct Index_DoIdMap {
  std::unique_ptr<IndexFile> current;
  std::unique_ptr<IndexFile> previous;

  PerformanceImportFile perf;
  bool is_interactive = false;
  bool write_to_disk = false;
  bool load_previous = false;

  Index_DoIdMap(std::unique_ptr<IndexFile> current,
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

  PerformanceImportFile perf;
  bool is_interactive;
  bool write_to_disk;

  Index_OnIdMapped(PerformanceImportFile perf,
                   bool is_interactive,
                   bool write_to_disk);
};

struct Index_OnIndexed {
  IndexUpdate update;
  PerformanceImportFile perf;

  Index_OnIndexed(IndexUpdate& update, PerformanceImportFile perf);
};

struct QueueManager {
  static QueueManager* instance();
  static void CreateInstance(MultiQueueWaiter* waiter);
  static void WriteStdout(IpcId id, lsBaseOutMessage& response);

  bool HasWork();

  // Runs on stdout thread.
  ThreadedQueue<Stdout_Request> for_stdout;
  // Runs on querydb thread.
  ThreadedQueue<std::unique_ptr<BaseIpcMessage>> for_querydb;

  // Runs on indexer threads.
  ThreadedQueue<Index_Request> index_request;
  ThreadedQueue<Index_DoIdMap> do_id_map;
  ThreadedQueue<Index_DoIdMap> load_previous_index;
  ThreadedQueue<Index_OnIdMapped> on_id_mapped;
  ThreadedQueue<Index_OnIndexed> on_indexed;

 private:
  explicit QueueManager(MultiQueueWaiter* waiter);

  static QueueManager* instance_;
};
