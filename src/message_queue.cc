#include "message_queue.h"

#include <cassert>
#include <functional>
#include <iostream>
#include <thread>

#include "platform.h"

namespace {

const int kMinimumPartialPayloadSize = 128;

struct BufferMessageIterator {
  static BufferMessageIterator Begin(void* buffer, size_t bytes_used) {
    if (bytes_used == 0)
      return End();

    return BufferMessageIterator(buffer, bytes_used);
  }
  static BufferMessageIterator End() {
    return BufferMessageIterator(nullptr, 0);
  }


  // Start of buffer to iterate.
  void* buffer;
  // Number of bytes left in buffer to parse.
  size_t remaining_bytes;

  BufferMessageIterator(void* buffer, size_t remaining_bytes)
    : buffer(buffer), remaining_bytes(remaining_bytes) {}

  Message* get() const {
    assert(buffer);
    return reinterpret_cast<Message*>(buffer);
  }
  Message* operator*() const { return get(); }
  Message* operator->() const { return get(); }

  void operator++() {
    size_t next_message_offset = get()->total_size;
    if (next_message_offset >= remaining_bytes) {
      assert(next_message_offset == remaining_bytes);
      buffer = nullptr;
      remaining_bytes = 0;
      return;
    }

    buffer = reinterpret_cast<char*>(buffer) + next_message_offset;
    remaining_bytes -= next_message_offset;
  }

  bool operator==(const BufferMessageIterator& other) const {
    return buffer == other.buffer && remaining_bytes == other.remaining_bytes;
  }
  bool operator!=(const BufferMessageIterator& other) const {
    return !(*this == other);
  }
};

enum class RepeatResult {
  RunAgain,
  Break
};

// Run |action| an arbitrary number of times.
void Repeat(std::function<RepeatResult()> action) {
  bool first = true;
  int log_iteration_count = 0;
  int log_count = 0;
  while (true) {
    if (!first) {
      if (log_iteration_count > 1000) {
        log_iteration_count = 0;
        std::cerr << "[info]: shmem full, waiting (" << log_count++ << ")" << std::endl; // TODO: remove
      }
      ++log_iteration_count;
      // TODO: See if we can figure out a way to use condition variables cross-process.
      std::this_thread::sleep_for(std::chrono::microseconds(0));
    }
    first = false;

    if (action() == RepeatResult::RunAgain)
      continue;
    break;
  }
}

}  // namespace

struct MessageQueue::BufferMetadata {
  // Total number of used bytes exluding the sizeof this metadata object.
  void set_total_messages_byte_count(size_t used_bytes) {
    total_message_bytes_ = used_bytes;
  }

  // The total number of bytes in use.
  size_t total_bytes_used_including_metadata() {
    return total_message_bytes_ + sizeof(BufferMetadata);
  }

  // The total number of bytes currently used for messages. This does not
  // include the sizeof the buffer metadata.
  size_t total_message_bytes() { return total_message_bytes_; }

  int next_partial_message_id = 0;

private:
  size_t total_message_bytes_ = 0;
};

MessageQueue::MessageQueue(std::unique_ptr<Buffer> buffer, bool buffer_has_data)
  : buffer_(std::move(buffer)) {
  if (!buffer_has_data)
    new (buffer_->data) BufferMetadata();
}

void MessageQueue::Enqueue(const Message& message) {
  //BufferMessageIterator::Begin(first_message_in_buffer(), metadata()->total_message_bytes);

  int partial_message_id = 0;

  Repeat([&]() {
    auto lock = buffer_->WaitForExclusiveAccess();

    // We cannot find the entire payload in the buffer. We have to send chunks
    // of it over time.
    if (message.total_size >= BytesAvailableInBuffer()) {
      // There's not enough room for our minimum payload size, so try again later.
      if ((sizeof(Message) + kMinimumPartialPayloadSize) > BytesAvailableInBuffer())
        return RepeatResult::RunAgain;

      if (partial_message_id == 0)
        partial_message_id = ++metadata()->next_partial_message_id; // note: pre-increment so we 1 as initial value


      size_t sent_payload_size = BytesAvailableInBuffer() - sizeof(Message);
      free_message_in_buffer()->CopyFrom(message);
      metadata()->set_total_messages_byte_count(
        metadata()->total_message_bytes() + sizeof(Message) + sent_payload_size);

      //shared_buffer->free_message()->Setup(message->ipc_id, partial_message_id, true /*has_more_chunks*/, sent_payload_size, payload);
      //shared_buffer->metadata()->bytes_used += sizeof(JsonMessage) + sent_payload_size;
      //shared_buffer->free_message()->ipc_id = IpcId::Invalid; // Note: free_message() may be past writable memory.

      if (count++ > 50) {
        std::cerr << "x50 Sending partial message with payload_size=" << sent_payload_size << std::endl;
        count = 0;
      }

      // Prepare for next time.
      payload_size -= sent_payload_size;
      payload += sent_payload_size;
      return RepeatResult::RunAgain;
    }

    return RepeatResult::Break;

#if false
    assert(payload_size > 0);

    // We cannot find the entire payload in the buffer. We
    // have to send chunks of it over time.
    if ((sizeof(JsonMessage) + payload_size) > shared_buffer->bytes_available()) {
      if ((sizeof(JsonMessage) + kMinimumPartialPayloadSize) > shared_buffer->bytes_available())
        return DispatchResult::RunAgain;

      if (partial_message_id == 0)
        partial_message_id = ++shared_buffer->metadata()->next_partial_message_id; // note: pre-increment so we 1 as initial value

      size_t sent_payload_size = shared_buffer->bytes_available() - sizeof(JsonMessage);
      shared_buffer->free_message()->Setup(message->ipc_id, partial_message_id, true /*has_more_chunks*/, sent_payload_size, payload);
      shared_buffer->metadata()->bytes_used += sizeof(JsonMessage) + sent_payload_size;
      //shared_buffer->free_message()->ipc_id = IpcId::Invalid; // Note: free_message() may be past writable memory.

      if (count++ > 50) {
        std::cerr << "x50 Sending partial message with payload_size=" << sent_payload_size << std::endl;
        count = 0;
    }

      // Prepare for next time.
      payload_size -= sent_payload_size;
      payload += sent_payload_size;
      return RepeatResult::RunAgain;
  }
    // The entire payload fits. Send it all now.
    else {
      // Include partial message id, as there could have been previous parts of this payload.
      shared_buffer->free_message()->Setup(message->ipc_id, partial_message_id, false /*has_more_chunks*/, payload_size, payload);
      shared_buffer->metadata()->bytes_used += sizeof(JsonMessage) + payload_size;
      shared_buffer->free_message()->ipc_id = IpcId::Invalid;
      //std::cerr << "Sending full message with payload_size=" << payload_size << std::endl;

      return RepeatResult::Break;
    }
#endif
});
}

MessageQueue::BufferMetadata* MessageQueue::metadata() const {
  return reinterpret_cast<BufferMetadata*>(buffer_->data);
}

size_t MessageQueue::BytesAvailableInBuffer() const {
  return buffer_->capacity - metadata()->total_bytes_used_including_metadata();
}

Message* MessageQueue::first_message_in_buffer() const {
  return reinterpret_cast<Message*>(
    reinterpret_cast<uint8_t>(buffer_->data) + sizeof(BufferMetadata));
}

Message* MessageQueue::free_message_in_buffer() const {
  if (metadata()->total_bytes_used_including_metadata >= buffer_->capacity)
    return nullptr;
  return reinterpret_cast<Message*>(
    reinterpret_cast<uint8_t>(buffer_->data) + metadata()->total_bytes_used_including_metadata());
}