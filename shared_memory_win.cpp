#include <iostream>
#include <vector>
#include <memory>
#include <iostream>
#include <chrono>
#include <thread>

#include <Windows.h>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include "serializer.h"

using Writer = rapidjson::PrettyWriter<rapidjson::StringBuffer>;
using Reader = rapidjson::Document;

struct ProcessMutex {
  HANDLE mutex_ = INVALID_HANDLE_VALUE;

  ProcessMutex() {
    mutex_ = ::CreateMutex(nullptr, false /*initial_owner*/, "indexer_shmem_mutex");
    assert(GetLastError() != ERROR_INVALID_HANDLE);
  }

  ~ProcessMutex() {
    ::ReleaseMutex(mutex_);
    mutex_ = INVALID_HANDLE_VALUE;
  }
};

struct ScopedProcessLock {
  HANDLE mutex_;

  ScopedProcessLock(ProcessMutex* mutex) : mutex_(mutex->mutex_) {
    WaitForSingleObject(mutex_, INFINITE);
  }

  ~ScopedProcessLock() {
    ::ReleaseMutex(mutex_);
  }
};

// Messages are funky objects. They contain potentially variable amounts of
// data and are passed between processes. This means that they need to be
// fully relocatable, ie, it is possible to memmove them in memory to a
// completely different address.

// TODO: Let's just pipe JSON.

struct JsonMessage {
  enum class Kind {
    Invalid,
    CreateIndex,
    ImportIndex
  };

  Kind kind;
  size_t payload_size;

  const char* payload() {
    return reinterpret_cast<const char*>(this) + sizeof(JsonMessage);
  }
  void set_payload(size_t payload_size, const char* payload) {
    char* payload_dest = reinterpret_cast<char*>(this) + sizeof(JsonMessage);
    this->payload_size = payload_size;
    memcpy(payload_dest, payload, payload_size);
  }
};

struct BaseMessage {
  JsonMessage::Kind kind;

  virtual void Serialize(Writer& writer) = 0;
  virtual void Deserialize(Reader& reader) = 0;
};



struct Message_ImportIndex : public BaseMessage {
  std::string path;

  Message_ImportIndex() {
    kind = JsonMessage::Kind::ImportIndex;
  }

  // BaseMessage:
  void Serialize(Writer& writer) override {
    writer.StartObject();
    ::Serialize(writer, "path", path);
    writer.EndObject();
  }
  void Deserialize(Reader& reader) override {
    ::Deserialize(reader, "path", path);
  }
};



struct Message_CreateIndex : public BaseMessage {
  std::string path;
  std::vector<std::string> args;

  Message_CreateIndex() {
    kind = JsonMessage::Kind::CreateIndex;
  }

  // BaseMessage:
  void Serialize(Writer& writer) override {
    writer.StartObject();
    ::Serialize(writer, "path", path);
    ::Serialize(writer, "args", args);
    writer.EndObject();
  }
  void Deserialize(Reader& reader) override {
    ::Deserialize(reader, "path", path);
    ::Deserialize(reader, "args", args);
  }
};


const int shmem_size = 1024;  // number of chars/bytes (256kb)

struct PlatformSharedMemory {
  HANDLE shmem_;
  void* shared_start_real_;


  size_t* shared_bytes_used;
  char* shared_start;


  PlatformSharedMemory() {
    shmem_ = ::CreateFileMapping(
      INVALID_HANDLE_VALUE,
      NULL,
      PAGE_READWRITE,
      0,
      shmem_size,
      "shared_memory_name"
    );

    shared_start_real_ = MapViewOfFile(shmem_, FILE_MAP_ALL_ACCESS, 0, 0, shmem_size);

    shared_bytes_used = reinterpret_cast<size_t*>(shared_start_real_);
    *shared_bytes_used = 0;
    shared_start = reinterpret_cast<char*>(shared_bytes_used + 1);
  }

  ~PlatformSharedMemory() {
    ::UnmapViewOfFile(shared_start_real_);
  }
};

struct MessageMemoryBlock {
  JsonMessage* ToMessage(char* ptr) {
    return reinterpret_cast<JsonMessage*>(ptr);
  }
  JsonMessage* get_free_message() {
    return reinterpret_cast<JsonMessage*>(shared.shared_start + *shared.shared_bytes_used);
  }

  // NOTE: We keep all pointers in terms of char* so pointer arithmetic is
  // always relative to bytes.

  // Pointers to shared memory.
  PlatformSharedMemory shared;

  ProcessMutex mutex;

  char* local_block;

  MessageMemoryBlock() {
    local_block = new char[shmem_size];
  }
  ~MessageMemoryBlock() {
    delete[] local_block;
  }



  void PushMessage(BaseMessage* message) {
    rapidjson::StringBuffer output;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output);
    writer.SetFormatOptions(
      rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
    writer.SetIndent(' ', 2);
    message->Serialize(writer);

    size_t payload_size = strlen(output.GetString());
    assert(payload_size < shmem_size && "Increase shared memory size, payload will never fit");

    bool first = true;
    bool did_log = false;
    while (true) {
      using namespace std::chrono_literals;
      if (!first) {
        if (!did_log) {
          std::cout << "[info]: shmem full, waiting" << std::endl; // TODO: remove
          did_log = true;
        }
        std::this_thread::sleep_for(16ms);
      }
      first = false;

      ScopedProcessLock lock(&mutex);

      // Try again later when there is room in shared memory.
      if ((*shared.shared_bytes_used + sizeof(JsonMessage) + payload_size) >= shmem_size)
        continue;

      get_free_message()->kind = message->kind;
      get_free_message()->set_payload(payload_size, output.GetString());

      *shared.shared_bytes_used += sizeof(JsonMessage) + get_free_message()->payload_size;
      assert(*shared.shared_bytes_used < shmem_size);
      get_free_message()->kind = JsonMessage::Kind::Invalid;
      break;
    }

  }



  std::vector<std::unique_ptr<BaseMessage>> PopMessage() {
    size_t remaining_bytes = 0;
    // Move data from shared memory into a local buffer. Do this
    // before parsing the blocks so that other processes can begin
    // posting data as soon as possible.
    {
      ScopedProcessLock lock(&mutex);
      remaining_bytes = *shared.shared_bytes_used;

      memcpy(local_block, shared.shared_start, *shared.shared_bytes_used);
      *shared.shared_bytes_used = 0;
      get_free_message()->kind = JsonMessage::Kind::Invalid;
    }

    std::vector<std::unique_ptr<BaseMessage>> result;

    char* message = local_block;
    while (remaining_bytes > 0) {
      std::unique_ptr<BaseMessage> base_message;
      switch (ToMessage(message)->kind) {
      case JsonMessage::Kind::CreateIndex:
        base_message = std::make_unique<Message_CreateIndex>();
        break;
      case JsonMessage::Kind::ImportIndex:
        base_message = std::make_unique<Message_ImportIndex>();
        break;
      default:
        assert(false);
      }

      rapidjson::Document document;
      document.Parse(ToMessage(message)->payload(), ToMessage(message)->payload_size);
      bool has_error = document.HasParseError();
      auto error = document.GetParseError();

      base_message->Deserialize(document);

      result.emplace_back(std::move(base_message));

      remaining_bytes -= sizeof(JsonMessage) + ToMessage(message)->payload_size;
      message = message + sizeof(JsonMessage) + ToMessage(message)->payload_size;
    }

    return result;
  }
};













void reader() {
  HANDLE shmem = INVALID_HANDLE_VALUE;
  HANDLE mutex = INVALID_HANDLE_VALUE;

  mutex = ::CreateMutex(NULL, FALSE, "mutex_sample_name");

  shmem = ::CreateFileMapping(
    INVALID_HANDLE_VALUE,
    NULL,
    PAGE_READWRITE,
    0,
    shmem_size,
    "shared_memory_name"
  );

  char *buf = (char*)MapViewOfFile(shmem, FILE_MAP_ALL_ACCESS, 0, 0, shmem_size);


  for (unsigned int c = 0; c < 60; ++c) {
    // mutex lock
    WaitForSingleObject(mutex, INFINITE);

    int value = buf[0];
    std::cout << "read shared memory...c=" << value << std::endl;

    // mutex unlock
    ::ReleaseMutex(mutex);

    ::Sleep(1000);
  }

  // release
  ::UnmapViewOfFile(buf);
  ::CloseHandle(shmem);
  ::ReleaseMutex(mutex);
}

void writer() {
  HANDLE	shmem = INVALID_HANDLE_VALUE;
  HANDLE	mutex = INVALID_HANDLE_VALUE;

  mutex = ::CreateMutex(NULL, FALSE, "mutex_sample_name");

  shmem = ::CreateFileMapping(
    INVALID_HANDLE_VALUE,
    NULL,
    PAGE_READWRITE,
    0,
    shmem_size,
    "shared_memory_name"
  );

  char *buf = (char*)::MapViewOfFile(shmem, FILE_MAP_ALL_ACCESS, 0, 0, shmem_size);

  for (unsigned int c = 0; c < 60; ++c) {
    // mutex lock
    WaitForSingleObject(mutex, INFINITE);

    // write shared memory
    memset(buf, c, shmem_size);

    std::cout << "write shared memory...c=" << c << std::endl;

    // mutex unlock
    ::ReleaseMutex(mutex);

    ::Sleep(1000);
  }

  // release
  ::UnmapViewOfFile(buf);
  ::CloseHandle(shmem);
  ::ReleaseMutex(mutex);
}

int main52525252(int argc, char** argv) {
  if (argc == 2)
    writer();
  else
    reader();

  return 0;
}


int main(int argc, char** argv) {
  if (argc == 2) {
    MessageMemoryBlock block;
    int i = 0;
    while (true) {
      Message_ImportIndex m;
      m.path = "foo #" + std::to_string(i);
      block.PushMessage(&m);
      std::cout << "Sent " << i << std::endl;;

      using namespace std::chrono_literals;
      std::this_thread::sleep_for(10ms);

      ++i;
    }
  }

  else {
    MessageMemoryBlock block;

    while (true) {
      std::vector<std::unique_ptr<BaseMessage>> messages = block.PopMessage();
      std::cout << "Got " << messages.size() << " messages" << std::endl;

      for (auto& message : messages) {
        rapidjson::StringBuffer output;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output);
        writer.SetFormatOptions(
          rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
        writer.SetIndent(' ', 2);
        message->Serialize(writer);

        std::cout << "  kind=" << static_cast<int>(message->kind) << ", json=" << output.GetString() << std::endl;
      }

      using namespace std::chrono_literals;
      std::this_thread::sleep_for(5s);
    }
  }

  return 0;
}