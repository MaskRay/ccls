#pragma once

#include "ipc.h"
#include "performance.h"
#include "query.h"
#include "threaded_queue.h"

#include <memory>

// TODO/FIXME: Merge IpcManager and QueueManager.

struct lsBaseOutMessage;

struct IpcManager {
  struct StdoutMessage {
    IpcId id;
    std::string content;
  };

  ThreadedQueue<StdoutMessage> for_stdout;
  ThreadedQueue<std::unique_ptr<BaseIpcMessage>> for_querydb;

  static IpcManager* instance();
  static void CreateInstance(MultiQueueWaiter* waiter);

  static void WriteStdout(IpcId id, lsBaseOutMessage& response);

 private:
  explicit IpcManager(MultiQueueWaiter* waiter);

  static IpcManager* instance_;
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
  using Index_RequestQueue = ThreadedQueue<Index_Request>;
  using Index_DoIdMapQueue = ThreadedQueue<Index_DoIdMap>;
  using Index_OnIdMappedQueue = ThreadedQueue<Index_OnIdMapped>;
  using Index_OnIndexedQueue = ThreadedQueue<Index_OnIndexed>;

  Index_RequestQueue index_request;
  Index_DoIdMapQueue do_id_map;
  Index_DoIdMapQueue load_previous_index;
  Index_OnIdMappedQueue on_id_mapped;
  Index_OnIndexedQueue on_indexed;

  QueueManager(MultiQueueWaiter* waiter);

  bool HasWork();
};
