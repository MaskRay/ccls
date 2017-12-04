#pragma once

#include "ipc.h"
#include "threaded_queue.h"

#include <memory>

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
