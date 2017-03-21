#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

#include "buffer.h"

class ResizableBuffer;

struct Message {
  Message(void* data, size_t size);

  void* data;
  size_t size;
};

// A MessageQueue is a FIFO container storing messages in an arbitrary memory
// buffer that is cross-thread and cross-process safe. This means:
//  - Multiple separate MessageQueues instantiations can point to the
//    same underlying buffer and use it at the same time.
//  - The buffer is fully relocatable, ie, it can have multiple different
//    addresses (as is the case for memory shared across processes).
//
// There can be multiple writers, but there can only be one reader.
struct MessageQueue {
  // Create a new MessageQueue using |buffer| as the backing data storage.
  // This does *not* take ownership over the memory stored in |buffer|.
  //
  // If |buffer_has_data| is true, then it is assumed that |buffer| contains
  // data and has already been initialized. It is a perfectly acceptable
  // use-case to have multiple completely separate MessageQueue
  // instantiations pointing to the same memory.
  explicit MessageQueue(std::unique_ptr<Buffer> buffer, bool buffer_has_data);
  MessageQueue(const MessageQueue&) = delete;

  // Enqueue a message to the queue. This will wait until there is room in
  // queue. If the message is too large to fit into the queue, this will
  // wait until the message has been fully sent, which may involve multiple
  // IPC roundtrips (ie, Enqueue -> DequeueAll -> Enqueue) - so this method
  // may take a long time to run.
  //
  // TODO: Consider copying message memory to a temporary buffer and running
  // enqueues on a worker thread.
  void Enqueue(const Message& message);

  // Take all messages from the queue.
  std::vector<std::unique_ptr<Buffer>> DequeueAll();

 private:
  struct BufferMetadata;

  void CopyPayloadToBuffer(uint32_t partial_id, void* payload, size_t payload_size, bool has_more_chunks);

  BufferMetadata* metadata() const;
  // Returns the number of bytes currently available in the buffer.
  size_t BytesAvailableInBuffer() const;
  Message* first_message_in_buffer() const;
  // First free message in the buffer.
  void* first_free_address_in_buffer() const;

  std::unique_ptr<Buffer> buffer_;
  std::unique_ptr<Buffer> local_buffer_;
};

/*
// TODO: We convert IpcMessage <-> Message as a user-level operation.
// MessageQueue doesn't know about IpcMessage.
struct IpcMessage {
  std::unique_ptr<Message> ToMessage();
  void BuildFromMessage(std::unique_ptr<Message> message);

  // Serialize/deserialize the message.
  virtual void Serialize(Writer& writer) = 0;
  virtual void Deserialize(Reader& reader) = 0;
};
*/