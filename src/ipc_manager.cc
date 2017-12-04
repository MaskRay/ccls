#include "ipc_manager.h"

#include "language_server_api.h"

#include <sstream>

IpcManager* IpcManager::instance_ = nullptr;

// static
IpcManager* IpcManager::instance() {
  return instance_;
}

// static
void IpcManager::CreateInstance(MultiQueueWaiter* waiter) {
  instance_ = new IpcManager(waiter);
}

// static
void IpcManager::WriteStdout(IpcId id, lsBaseOutMessage& response) {
  std::ostringstream sstream;
  response.Write(sstream);

  StdoutMessage out;
  out.content = sstream.str();
  out.id = id;
  instance()->for_stdout.Enqueue(std::move(out));
}

IpcManager::IpcManager(MultiQueueWaiter* waiter)
    : for_stdout(waiter), for_querydb(waiter) {}