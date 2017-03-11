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

// TODO: We need to add support for payloads larger than the maximum shared memory buffer size.


using IpcMessageId = std::string;

struct BaseIpcMessageElided {
  virtual IpcMessageId runtime_id() const = 0;
  virtual int hashed_runtime_id() const = 0;

  virtual void Serialize(Writer& writer);
  virtual void Deserialize(Reader& reader);
};

// Usage:
//
//  class IpcMessage_Foo : public BaseIpcMessage<IpcMessage_Foo> {
//    static IpcMessageId kId;
//
//    // BaseIpcMessage:
//    ...
//  }
//  IpcMessageId IpcMessage_Foo::kId = "Foo";
//
//
//  main() {
//    IpcRegistry::instance()->Register<IpcMessage_Foo>();
//  }
//
// Note: This is a template so that the statics are stored separately
// per type.
template<typename T>
struct BaseIpcMessage : BaseIpcMessageElided {
  BaseIpcMessage();
  virtual ~BaseIpcMessage();

  // Populated by IpcRegistry::RegisterAllocator.
  static IpcMessageId runtime_id_;
  static int hashed_runtime_id_;

  // BaseIpcMessageElided:
  IpcMessageId runtime_id() const override {
    return runtime_id_;
  }
  int hashed_runtime_id() const override {
    return hashed_runtime_id_;
  }
};

struct IpcRegistry {
  using Allocator = std::function<BaseIpcMessageElided*()>;

  // Use unique_ptrs so we can initialize on first use
  // (static init order might not be right).
  std::unique_ptr<std::unordered_map<int, Allocator>> allocators;
  std::unique_ptr<std::unordered_map<int, std::string>> hash_to_id;

  template<typename T>
  void Register();

  std::unique_ptr<BaseIpcMessageElided> Allocate(int id);

  static IpcRegistry* instance() {
    if (!instance_)
      instance_ = new IpcRegistry();
    return instance_;
  }
  static IpcRegistry* instance_;
};

template<typename T>
void IpcRegistry::Register() {
  if (!allocators) {
    allocators = MakeUnique<std::unordered_map<int, Allocator>>();
    hash_to_id = MakeUnique<std::unordered_map<int, std::string>>();
  }

  IpcMessageId id = T::kId;

  int hash = std::hash<IpcMessageId>()(id);
  auto it = allocators->find(hash);
  assert(allocators->find(hash) == allocators->end() && "There is already an IPC message with the given id");

  (*hash_to_id)[hash] = id;
  (*allocators)[hash] = []() {
    return new T();
  };

  T::runtime_id_ = id;
  T::hashed_runtime_id_ = hash;
}






struct IpcDirectionalChannel {
  // NOTE: We keep all pointers in terms of char* so pointer arithmetic is
  // always relative to bytes.

  explicit IpcDirectionalChannel(const std::string& name);
  ~IpcDirectionalChannel();

  void PushMessage(BaseIpcMessageElided* message);
  std::vector<std::unique_ptr<BaseIpcMessageElided>> TakeMessages();

  // Pointer to process shared memory and process shared mutex.
  std::unique_ptr<PlatformSharedMemory> shared;
  std::unique_ptr<PlatformMutex> mutex;

  // Pointer to process-local memory.
  char* local_block;
};

struct IpcServer {
  IpcServer(const std::string& name);

  void SendToClient(int client_id, BaseIpcMessageElided* message);
  std::vector<std::unique_ptr<BaseIpcMessageElided>> TakeMessages();

private:
  std::string name_;
  IpcDirectionalChannel server_;
  std::unordered_map<int, std::unique_ptr<IpcDirectionalChannel>> clients_;
};

struct IpcClient {
  IpcClient(const std::string& name, int client_id);

  void SendToServer(BaseIpcMessageElided* message);
  std::vector<std::unique_ptr<BaseIpcMessageElided>> TakeMessages();

  IpcDirectionalChannel* client() { return &client_; }

private:
  IpcDirectionalChannel server_;
  IpcDirectionalChannel client_;
};











template<typename T>
BaseIpcMessage<T>::BaseIpcMessage() {
  assert(!runtime_id_.empty() && "Message is not registered using IpcRegistry::RegisterAllocator");
}

template<typename T>
BaseIpcMessage<T>::~BaseIpcMessage() {}

template<typename T>
IpcMessageId BaseIpcMessage<T>::runtime_id_;

template<typename T>
int BaseIpcMessage<T>::hashed_runtime_id_ = -1;
