#include "ipc_manager.h"

IpcManager* IpcManager::instance_ = nullptr;

// static 
IpcManager* IpcManager::instance() {
  return instance_;
}

// static
void IpcManager::CreateInstance(MultiQueueWaiter* waiter) {
  instance_ = new IpcManager(waiter);
}

ThreadedQueue<std::unique_ptr<BaseIpcMessage>>* IpcManager::GetThreadedQueue(Destination destination) {
  return destination == Destination::Client ? threaded_queue_for_client_.get() : threaded_queue_for_server_.get();
}

void IpcManager::SendOutMessageToClient(IpcId id, lsBaseOutMessage& response) {
  std::ostringstream sstream;
  response.Write(sstream);

  auto out = MakeUnique<Ipc_Cout>();
  out->content = sstream.str();
  out->original_ipc_id = id;
  GetThreadedQueue(Destination::Client)->Enqueue(std::move(out));
}

void IpcManager::SendMessage(Destination destination, std::unique_ptr<BaseIpcMessage> message) {
  GetThreadedQueue(destination)->Enqueue(std::move(message));
}

std::vector<std::unique_ptr<BaseIpcMessage>> IpcManager::GetMessages(Destination destination) {
  return GetThreadedQueue(destination)->DequeueAll();
}

IpcManager::IpcManager(MultiQueueWaiter* waiter) {
  threaded_queue_for_client_ = MakeUnique<ThreadedQueue<std::unique_ptr<BaseIpcMessage>>>(waiter);
  threaded_queue_for_server_ = MakeUnique<ThreadedQueue<std::unique_ptr<BaseIpcMessage>>>(waiter);
}