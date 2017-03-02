#pragma once

#include <iostream>
#include <chrono>
#include <string>
#include <thread>

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
  enum class Kind {
    Invalid,
    IsAlive,
    CreateIndex,
    ImportIndex,
  };

  Kind kind;
  size_t payload_size;

  const char* payload();
  void SetPayload(size_t payload_size, const char* payload);
};

struct BaseIpcMessage {
  JsonMessage::Kind kind;
  virtual ~BaseIpcMessage() {}

  virtual void Serialize(Writer& writer) = 0;
  virtual void Deserialize(Reader& reader) = 0;
};

struct IpcMessage_IsAlive : public BaseIpcMessage {
  IpcMessage_IsAlive();

  // BaseIpcMessage:
  void Serialize(Writer& writer) override;
  void Deserialize(Reader& reader) override;
};

struct IpcMessage_ImportIndex : public BaseIpcMessage {
  std::string path;

  IpcMessage_ImportIndex();

  // BaseMessage:
  void Serialize(Writer& writer) override;
  void Deserialize(Reader& reader) override;
};

struct IpcMessage_CreateIndex : public BaseIpcMessage {
  std::string path;
  std::vector<std::string> args;

  IpcMessage_CreateIndex();

  // BaseMessage:
  void Serialize(Writer& writer) override;
  void Deserialize(Reader& reader) override;
};

struct IpcMessageQueue {
  // NOTE: We keep all pointers in terms of char* so pointer arithmetic is
  // always relative to bytes.

  explicit IpcMessageQueue(const std::string& name);
  ~IpcMessageQueue();

  void PushMessage(BaseIpcMessage* message);
  std::vector<std::unique_ptr<BaseIpcMessage>> PopMessage();

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