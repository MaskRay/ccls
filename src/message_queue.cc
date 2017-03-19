#include "message_queue.h"

#include <cassert>

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
  size_t total_message_bytes() {
    return total_message_bytes_;
  }

private:
  size_t total_message_bytes_ = 0;
};

MessageQueue::MessageQueue(std::unique_ptr<Buffer> buffer, bool buffer_has_data) : buffer_(std::move(buffer)) {
  if (!buffer_has_data)
    new(buffer_->data) BufferMetadata();
}

void MessageQueue::Enqueue(const Message& message) {

}

MessageQueue::BufferMetadata* MessageQueue::Metadata() {
  return reinterpret_cast<BufferMetadata*>(buffer_->data);
}