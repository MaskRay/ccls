#include "ipc.h"
#include "serializer.h"

namespace {
  // JSON-encoded message that is passed across shared memory.
  //
  // Messages are funky objects. They contain potentially variable amounts of
  // data and are passed between processes. This means that they need to be
  // fully relocatable, ie, it is possible to memmove them in memory to a
  // completely different address.
  struct JsonMessage {
    IpcId ipc_id;
    size_t payload_size;

    const char* payload() {
      return reinterpret_cast<const char*>(this) + sizeof(JsonMessage);
    }
    void SetPayload(size_t payload_size, const char* payload) {
      char* payload_dest = reinterpret_cast<char*>(this) + sizeof(JsonMessage);
      this->payload_size = payload_size;
      memcpy(payload_dest, payload, payload_size);
    }
  };

  std::string NameToServerName(const std::string& name) {
    return name + "server";
  }

  std::string NameToClientName(const std::string& name, int client_id) {
    return name + "client" + std::to_string(client_id);
  }
}

IpcRegistry* IpcRegistry::instance_ = nullptr;

std::unique_ptr<IpcMessage> IpcRegistry::Allocate(IpcId id) {
  return std::unique_ptr<IpcMessage>((*allocators)[id]());
}

struct IpcDirectionalChannel::MessageBuffer {
  MessageBuffer(void* buffer, size_t buffer_size) {
    real_buffer = buffer;
    real_buffer_size = buffer_size;
    new(real_buffer) Metadata();
  }

  // Pointer to the start of the actual buffer and the
  // amount of storage actually available.
  void* real_buffer;
  size_t real_buffer_size;

  template<typename T>
  T* Offset(size_t offset) {
    return static_cast<T>(static_cast<char*>(real_buffer) + offset);
  }

  // Number of bytes available in buffer_start. Note that this
  // is smaller than the total buffer size, since there is some
  // metadata stored at the start of the buffer.
  size_t buffer_size;

  struct Metadata {
    // The number of bytes that are currently used in the buffer minus the
    // size of this Metadata struct.
    size_t bytes_used = 0;
    int next_partial_message_id = 0;
    int num_outstanding_partial_messages = 0;
  };

  Metadata* metadata() {
    return Offset<Metadata>(0);
  }

  // First json message.
  JsonMessage* first_message() {
    return Offset<JsonMessage>(sizeof(Metadata));
  }

  // First free, writable json message. Make sure to increase *bytes_used()
  // by any written size.
  JsonMessage* free_message() {
    return Offset<JsonMessage>(sizeof(Metadata) + metadata()->bytes_used);
  }
};

IpcDirectionalChannel::IpcDirectionalChannel(const std::string& name) {
  shared = CreatePlatformSharedMemory(name + "memory");
  mutex = CreatePlatformMutex(name + "mutex");
  local = std::unique_ptr<void>(new char[shmem_size]);
 
  shared_buffer = MakeUnique<MessageBuffer>(shared->shared);
  local_buffer = MakeUnique<MessageBuffer>(local.get());
}

IpcDirectionalChannel::~IpcDirectionalChannel() {}

// TODO:
//  We need to send partial payloads. But other payloads may appear in
//  the middle of the payload.
//
//  
//  int partial_payload_id = 0
//  int num_uncompleted_payloads = 0

void IpcDirectionalChannel::PushMessage(IpcMessage* message) {
  assert(message->ipc_id != IpcId::Invalid);

  rapidjson::StringBuffer output;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output);
  writer.SetFormatOptions(
    rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
  writer.SetIndent(' ', 2);
  message->Serialize(writer);

  //std::cerr << "Sending message with id " << message->runtime_id() << " (hash " << message->hashed_runtime_id() << ")" << std::endl;

  size_t payload_size = strlen(output.GetString());
  assert(payload_size < shmem_size && "Increase shared memory size, payload will never fit");

  bool first = true;
  bool did_log = false;
  while (true) {
    if (!first) {
      if (!did_log) {
        std::cerr << "[info]: shmem full, waiting" << std::endl; // TODO: remove
        did_log = true;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    first = false;

    std::unique_ptr<PlatformScopedMutexLock> lock = CreatePlatformScopedMutexLock(mutex.get());

    // Try again later when there is room in shared memory.
    if ((shared_buffer->metadata()->bytes_used + sizeof(MessageBuffer::Metadata) + sizeof(JsonMessage) + payload_size) >= shmem_size)
      continue;

    shared_buffer->free_message()->ipc_id = message->ipc_id;
    shared_buffer->free_message()->SetPayload(payload_size, output.GetString());

    shared_buffer->metadata()->bytes_used += sizeof(JsonMessage) + shared_buffer->free_message()->payload_size;
    assert((shared_buffer->metadata()->bytes_used + sizeof(MessageBuffer::Metadata)) < shmem_size);
    shared_buffer->free_message()->ipc_id = IpcId::Invalid;
    break;
  }

}

std::vector<std::unique_ptr<IpcMessage>> IpcDirectionalChannel::TakeMessages() {
  size_t remaining_bytes = 0;
  // Move data from shared memory into a local buffer. Do this
  // before parsing the blocks so that other processes can begin
  // posting data as soon as possible.
  {
    std::unique_ptr<PlatformScopedMutexLock> lock = CreatePlatformScopedMutexLock(mutex.get());
    remaining_bytes = *shared->shared_bytes_used;

    memcpy(local_block, shared->shared_start, *shared->shared_bytes_used);
    *shared->shared_bytes_used = 0;
    get_free_message(this)->ipc_id = IpcId::Invalid;
  }

  std::vector<std::unique_ptr<IpcMessage>> result;

  char* message = local_block;
  while (remaining_bytes > 0) {
    std::unique_ptr<IpcMessage> base_message = IpcRegistry::instance()->Allocate(as_message(message)->ipc_id);

    rapidjson::Document document;
    document.Parse(as_message(message)->payload(), as_message(message)->payload_size);
    bool has_error = document.HasParseError();
    auto error = document.GetParseError();

    base_message->Deserialize(document);

    result.emplace_back(std::move(base_message));

    remaining_bytes -= sizeof(JsonMessage) + as_message(message)->payload_size;
    message = message + sizeof(JsonMessage) + as_message(message)->payload_size;
  }

  return result;
}



IpcServer::IpcServer(const std::string& name)
  : name_(name), server_(NameToServerName(name)) {}

void IpcServer::SendToClient(int client_id, IpcMessage* message) {
  // Find or create the client.
  auto it = clients_.find(client_id);
  if (it == clients_.end())
    clients_[client_id] = MakeUnique<IpcDirectionalChannel>(NameToClientName(name_, client_id));

  clients_[client_id]->PushMessage(message);
}

std::vector<std::unique_ptr<IpcMessage>> IpcServer::TakeMessages() {
  return server_.TakeMessages();
}

IpcClient::IpcClient(const std::string& name, int client_id)
  : server_(NameToServerName(name)), client_(NameToClientName(name, client_id)) {}

void IpcClient::SendToServer(IpcMessage* message) {
  server_.PushMessage(message);
}

std::vector<std::unique_ptr<IpcMessage>> IpcClient::TakeMessages() {
  return client_.TakeMessages();
}
