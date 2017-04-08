#pragma once

#include <functional>
#include <iostream>
#include <unordered_map>

#include "buffer.h"
#include "message_queue.h"
#include "serializer.h"

// TypedBidiMessageQueue provides a type-safe server/client implementation on
// top of a couple MessageQueue instances.
template <typename TId, typename TMessage>
struct TypedBidiMessageQueue {
  using Serializer = std::function<void(Writer& visitor, TMessage& message)>;
  using Deserializer =
      std::function<std::unique_ptr<TMessage>(Reader& visitor)>;

  TypedBidiMessageQueue(const std::string& name, size_t buffer_size)
      : for_server(
            Buffer::CreateSharedBuffer(name + "_fs", buffer_size),
            false /*buffer_has_data*/),
        for_client(
            Buffer::CreateSharedBuffer(name + "_fc", buffer_size),
            true /*buffer_has_data*/) {}

  void RegisterId(TId id,
                  const Serializer& serializer,
                  const Deserializer& deserializer) {
    assert(serializers_.find(id) == serializers_.end() &&
           deserializers_.find(id) == deserializers_.end() &&
           "Duplicate registration");

    serializers_[id] = serializer;
    deserializers_[id] = deserializer;
  }

  void SendMessage(MessageQueue* destination, TId id, TMessage& message) {
    // Create writer.
    rapidjson::StringBuffer output;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output);
    writer.SetIndent(' ', 0);

    // Serialize the message.
    assert(serializers_.find(id) != serializers_.end() && "No registered serializer");
    const Serializer& serializer = serializers_.find(id)->second;
    serializer(writer, message);

    // Send message.
    void* payload = malloc(sizeof(MessageHeader) + output.GetSize());
    reinterpret_cast<MessageHeader*>(payload)->id = id;
    memcpy(
        (void*)(reinterpret_cast<const char*>(payload) + sizeof(MessageHeader)),
        output.GetString(), output.GetSize());
    destination->Enqueue(
        Message(payload, sizeof(MessageHeader) + output.GetSize()));
    free(payload);
  }

  // Retrieve all messages from the given |queue|.
  std::vector<std::unique_ptr<TMessage>> GetMessages(
      MessageQueue* queue) const {
    assert(queue == &for_server || queue == &for_client);

    std::vector<std::unique_ptr<Buffer>> messages = queue->DequeueAll();
    std::vector<std::unique_ptr<TMessage>> result;
    result.reserve(messages.size());

    for (std::unique_ptr<Buffer>& buffer : messages) {
      MessageHeader* header = reinterpret_cast<MessageHeader*>(buffer->data);

      // Parse message content.
      rapidjson::Document document;
      document.Parse(
          reinterpret_cast<const char*>(buffer->data) + sizeof(MessageHeader),
          buffer->capacity - sizeof(MessageHeader));
      if (document.HasParseError()) {
        std::cerr << "[FATAL]: Unable to parse IPC message" << std::endl;
        exit(1);
      }

      // Deserialize it.
      assert(deserializers_.find(header->id) != deserializers_.end() &&
             "No registered deserializer");
      const Deserializer& deserializer =
          deserializers_.find(header->id)->second;
      result.emplace_back(deserializer(document));
    }

    return result;
  }

  // Messages which the server process should handle.
  MessageQueue for_server;
  // Messages which the client process should handle.
  MessageQueue for_client;

 private:
  struct MessageHeader {
    TId id;
  };

  std::unordered_map<TId, Serializer> serializers_;
  std::unordered_map<TId, Deserializer> deserializers_;
};
