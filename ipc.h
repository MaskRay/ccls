#pragma once

#include <iostream>
#include <chrono>
#include <string>
#include <thread>
#include <unordered_map>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include "src/platform.h"
#include "serializer.h"
#include "utils.h"

// TODO: We need to add support for payloads larger than the maximum shared memory buffer size.


enum class IpcId : int {
  // Invalid request id.
  Invalid = 0,

  Quit = 1,
  IsAlive,
  OpenProject,

  IndexTranslationUnitRequest,
  IndexTranslationUnitResponse,

  // This is a language server request. The actual request method
  // id is embedded within the request state.
  LanguageServerRequest,
  // TODO: remove
  DocumentSymbolsRequest,
  DocumentSymbolsResponse,
  DocumentCodeLensRequest,
  DocumentCodeLensResponse,
  CodeLensResolveRequest,
  CodeLensResolveResponse,
  WorkspaceSymbolsRequest,
  WorkspaceSymbolsResponse,

  Test
};

namespace std {
  template <>
  struct hash<IpcId> {
    size_t operator()(const IpcId& k) const {
      return hash<int>()(static_cast<int>(k));
    }
  };
}

struct IpcMessage {
  IpcMessage(IpcId ipc_id) : ipc_id(ipc_id) {}
  virtual ~IpcMessage() {}

  const IpcId ipc_id;

  virtual void Serialize(Writer& writer) = 0;
  virtual void Deserialize(Reader& reader) = 0;
};

struct IpcRegistry {
  using Allocator = std::function<IpcMessage*()>;

  // Use unique_ptrs so we can initialize on first use
  // (static init order might not be right).
  std::unique_ptr<std::unordered_map<IpcId, Allocator>> allocators_;

  template<typename T>
  void Register(IpcId id);

  std::unique_ptr<IpcMessage> Allocate(IpcId id);

  static IpcRegistry* instance() {
    if (!instance_)
      instance_ = new IpcRegistry();
    return instance_;
  }
  static IpcRegistry* instance_;
};

template<typename T>
void IpcRegistry::Register(IpcId id) {
  if (!allocators_)
    allocators_ = MakeUnique<std::unordered_map<IpcId, Allocator>>();

  assert(allocators_->find(id) == allocators_->end() &&
         "There is already an IPC message with the given id");

  (*allocators_)[id] = [id]() {
    return new T();
  };
}






struct IpcDirectionalChannel {
  // NOTE: We keep all pointers in terms of char* so pointer arithmetic is
  // always relative to bytes.

  explicit IpcDirectionalChannel(const std::string& name, bool initialize_shared_memory);
  ~IpcDirectionalChannel();

  void PushMessage(IpcMessage* message);
  std::vector<std::unique_ptr<IpcMessage>> TakeMessages();

  struct MessageBuffer;
  struct ResizableBuffer;

  ResizableBuffer* CreateOrFindResizableBuffer(int id);
  void RemoveResizableBuffer(int id);
  std::unordered_map<int, std::unique_ptr<ResizableBuffer>> resizable_buffers;

  // Pointer to process shared memory and process shared mutex.
  std::unique_ptr<PlatformSharedMemory> shared;
  std::unique_ptr<PlatformMutex> mutex;

  // Pointer to process-local memory.
  std::unique_ptr<char> local;

  std::unique_ptr<MessageBuffer> shared_buffer;
  std::unique_ptr<MessageBuffer> local_buffer;
};

struct IpcServer {
  IpcServer(const std::string& name, int num_clients);

  void SendToClient(int client_id, IpcMessage* message);
  std::vector<std::unique_ptr<IpcMessage>> TakeMessages();

  int num_clients() const { return clients_.size(); }

private:
  IpcDirectionalChannel server_; // Local / us.
  std::vector<std::unique_ptr<IpcDirectionalChannel>> clients_;
};

struct IpcClient {
  IpcClient(const std::string& name, int client_id);

  void SendToServer(IpcMessage* message);
  std::vector<std::unique_ptr<IpcMessage>> TakeMessages();

  IpcDirectionalChannel* client() { return &client_; }

private:
  IpcDirectionalChannel server_;
  IpcDirectionalChannel client_;
};
