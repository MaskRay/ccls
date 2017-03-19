#pragma once

#include <vector>
#include <memory>

#include "buffer.h"

struct Message {
  // Unique message identifier.
  uint8_t message_id;

  // Total size of the message (including metadata that this object stores).
  size_t total_size;
};

// A MessageQueue is a FIFO container storing messages in an arbitrary memory
// buffer.
//  - Multiple separate MessageQueues instantiations can point to the
//    same underlying buffer
//  - Buffer is fully relocatable, ie, it can have multiple different
//    addresses (as is the case for memory shared across processes).
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
  //
  // note:
  //  We could make this allocation free by returning raw pointers to the
  //  internal process-local buffer, but that is pretty haphazard and likely
  //  to cause a very confusing crash. The extra memory allocations here from
  //  unique_ptr going to make a performance difference.
  std::vector<std::unique_ptr<Message>> DequeueAll();

  // Take the first available message from the queue.
  std::unique_ptr<Message> DequeueFirst();

private:
  struct BufferMetadata;

  BufferMetadata* Metadata();

  std::unique_ptr<Buffer> buffer_;
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