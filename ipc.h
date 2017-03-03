#pragma once

#include <iostream>
#include <chrono>
#include <string>
#include <thread>
#include <unordered_map>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include "platform.h"
#include "serializer.h"

using Writer = rapidjson::PrettyWriter<rapidjson::StringBuffer>;
using Reader = rapidjson::Document;


// Messages are funky objects. They contain potentially variable amounts of
// data and are passed between processes. This means that they need to be
// fully relocatable, ie, it is possible to memmove them in memory to a
// completely different address.

struct JsonMessage {
  int message_id;
  size_t payload_size;

  const char* payload();
  void SetPayload(size_t payload_size, const char* payload);
};

using IpcMessageId = std::string;

struct BaseIpcMessage {
  virtual ~BaseIpcMessage();

  virtual void Serialize(Writer& writer);
  virtual void Deserialize(Reader& reader);

  IpcMessageId runtime_id;
  int hashed_runtime_id;

  /*
private:
  template<typename T>
  friend struct IpcMessage;
  */

  enum class DoNotDeriveDirectly {
    DeriveFromIpcMessageInstead
  };
  BaseIpcMessage(DoNotDeriveDirectly);
};

struct IpcRegistry {
  using Allocator = std::function<BaseIpcMessage*()>;

  // Use unique_ptrs so we can initialize on first use
  // (static init order might not be right).
  std::unique_ptr<std::unordered_map<int, Allocator>> allocators;
  std::unique_ptr<std::unordered_map<int, std::string>> hash_to_id;

  template<typename T>
  int RegisterAllocator();

  std::unique_ptr<BaseIpcMessage> Allocate(int id);

  static IpcRegistry* instance() {
    // TODO: Remove static magic. Just call register explicitly.
    return instance_;
  }
  static IpcRegistry* instance_;
};

template<typename T>
int IpcRegistry::RegisterAllocator() {
  if (!allocators) {
    allocators = std::make_unique<std::unordered_map<int, Allocator>>();
    hash_to_id = std::make_unique<std::unordered_map<int, std::string>>();
  }

  IpcMessageId id = T::id;

  int hash = std::hash<IpcMessageId>()(id);
  auto it = allocators->find(hash);
  assert(allocators->find(hash) == allocators->end() && "There is already an IPC message with the given id");

  (*hash_to_id)[hash] = id;
  (*allocators)[hash] = []() {
    return new T();
  };

  return hash;
}

template<typename TChild>
struct IpcMessage : public BaseIpcMessage {
  IpcMessage();

  static int hashed_id_;
};

template<typename TChild>
int IpcMessage<TChild>::hashed_id_ = IpcRegistry::Instance.RegisterAllocator<TChild>();

template<typename TChild>
IpcMessage<TChild>::IpcMessage()
  : BaseIpcMessage(DoNotDeriveDirectly::DeriveFromIpcMessageInstead) {
  runtime_id = TChild::id;
  hashed_runtime_id = hashed_id_;
}






struct IpcDirectionalChannel {
  // NOTE: We keep all pointers in terms of char* so pointer arithmetic is
  // always relative to bytes.

  explicit IpcDirectionalChannel(const std::string& name);
  ~IpcDirectionalChannel();

  void PushMessage(BaseIpcMessage* message);
  std::vector<std::unique_ptr<BaseIpcMessage>> TakeMessages();

private:
  JsonMessage* get_free_message() {
    return reinterpret_cast<JsonMessage*>(shared->shared_start + *shared->shared_bytes_used);
  }

  // Pointer to process shared memory and process shared mutex.
  std::unique_ptr<PlatformSharedMemory> shared;
  std::unique_ptr<PlatformMutex> mutex;

  // Pointer to process-local memory.
  char* local_block;
};

struct IpcServer {
  IpcServer(const std::string& name);

  void SendToClient(int client_id, BaseIpcMessage* message);
  std::vector<std::unique_ptr<BaseIpcMessage>> TakeMessages();

private:
  std::string name_;
  IpcDirectionalChannel server_;
  std::unordered_map<int, std::unique_ptr<IpcDirectionalChannel>> clients_;
};

struct IpcClient {
  IpcClient(const std::string& name, int client_id);

  void SendToServer(BaseIpcMessage* message);
  std::vector<std::unique_ptr<BaseIpcMessage>> TakeMessages();

private:
  IpcDirectionalChannel server_;
  IpcDirectionalChannel client_;
};