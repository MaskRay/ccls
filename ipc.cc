#include "ipc.h"

namespace {
  JsonMessage* as_message(char* ptr) {
    return reinterpret_cast<JsonMessage*>(ptr);
  }

  std::string NameToServerName(const std::string& name) {
    return name + "_server";
  }

  std::string NameToClientName(const std::string& name, int client_id) {
    return name + "_server_" + std::to_string(client_id);
  }
}

const char* JsonMessage::payload() {
  return reinterpret_cast<const char*>(this) + sizeof(JsonMessage);
}

void JsonMessage::SetPayload(size_t payload_size, const char* payload) {
  char* payload_dest = reinterpret_cast<char*>(this) + sizeof(JsonMessage);
  this->payload_size = payload_size;
  memcpy(payload_dest, payload, payload_size);
}

BaseIpcMessage::BaseIpcMessage(BaseIpcMessage::DoNotDeriveDirectly) {}

BaseIpcMessage::~BaseIpcMessage() {}

void BaseIpcMessage::Serialize(Writer& writer) {}

void BaseIpcMessage::Deserialize(Reader& reader) {}


IpcRegistry IpcRegistry::Instance;

std::unique_ptr<BaseIpcMessage> IpcRegistry::Allocate(int id) {
  return std::unique_ptr<BaseIpcMessage>((*allocators)[id]());
}

IpcDirectionalChannel::IpcDirectionalChannel(const std::string& name) {
  local_block = new char[shmem_size];
  shared = CreatePlatformSharedMemory(name + "_memory");
  mutex = CreatePlatformMutex(name + "_mutex");
}

IpcDirectionalChannel::~IpcDirectionalChannel() {
  delete[] local_block;
}

void IpcDirectionalChannel::PushMessage(BaseIpcMessage* message) {
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

    std::unique_ptr<PlatformScopedMutexLock> lock = CreatePlatformScopedMutexLock(mutex.get());

    // Try again later when there is room in shared memory.
    if ((*shared->shared_bytes_used + sizeof(JsonMessage) + payload_size) >= shmem_size)
      continue;

    get_free_message()->message_id = message->hashed_runtime_id;
    get_free_message()->SetPayload(payload_size, output.GetString());

    *shared->shared_bytes_used += sizeof(JsonMessage) + get_free_message()->payload_size;
    assert(*shared->shared_bytes_used < shmem_size);
    get_free_message()->message_id = -1;
    break;
  }

}

std::vector<std::unique_ptr<BaseIpcMessage>> IpcDirectionalChannel::TakeMessages() {
  size_t remaining_bytes = 0;
  // Move data from shared memory into a local buffer. Do this
  // before parsing the blocks so that other processes can begin
  // posting data as soon as possible.
  {
    std::unique_ptr<PlatformScopedMutexLock> lock = CreatePlatformScopedMutexLock(mutex.get());
    remaining_bytes = *shared->shared_bytes_used;

    memcpy(local_block, shared->shared_start, *shared->shared_bytes_used);
    *shared->shared_bytes_used = 0;
    get_free_message()->message_id = -1;
  }

  std::vector<std::unique_ptr<BaseIpcMessage>> result;

  char* message = local_block;
  while (remaining_bytes > 0) {
    std::unique_ptr<BaseIpcMessage> base_message = IpcRegistry::Instance.Allocate(as_message(message)->message_id);

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

void IpcServer::SendToClient(int client_id, BaseIpcMessage* message) {
  // Find or create the client.
  auto it = clients_.find(client_id);
  if (it == clients_.end())
    clients_[client_id] = std::make_unique<IpcDirectionalChannel>(NameToClientName(name_, client_id));

  clients_[client_id]->PushMessage(message);
}

std::vector<std::unique_ptr<BaseIpcMessage>> IpcServer::TakeMessages() {
  return server_.TakeMessages();
}

IpcClient::IpcClient(const std::string& name, int client_id)
  : server_(NameToServerName(name)), client_(NameToClientName(name, client_id)) {}

void IpcClient::SendToServer(BaseIpcMessage* message) {
  server_.PushMessage(message);
}

std::vector<std::unique_ptr<BaseIpcMessage>> IpcClient::TakeMessages() {
  return client_.TakeMessages();
}