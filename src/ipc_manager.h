#pragma once

#include "ipc.h"
#include "language_server_api.h"
#include "threaded_queue.h"

#include <memory>

struct IpcManager {
  static IpcManager* instance_;
  static IpcManager* instance();
  static void CreateInstance(MultiQueueWaiter* waiter);

  std::unique_ptr<ThreadedQueue<std::unique_ptr<BaseIpcMessage>>>
      threaded_queue_for_client_;
  std::unique_ptr<ThreadedQueue<std::unique_ptr<BaseIpcMessage>>>
      threaded_queue_for_server_;

  enum class Destination { Client, Server };

  ThreadedQueue<std::unique_ptr<BaseIpcMessage>>* GetThreadedQueue(
      Destination destination);

  void SendOutMessageToClient(IpcId id, lsBaseOutMessage& response);

  void SendMessage(Destination destination,
                   std::unique_ptr<BaseIpcMessage> message);

  std::vector<std::unique_ptr<BaseIpcMessage>> GetMessages(
      Destination destination);

 private:
  IpcManager(MultiQueueWaiter* waiter);
};
