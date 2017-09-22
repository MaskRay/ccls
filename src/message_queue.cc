#include "message_queue.h"

#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include <thread>

#include <doctest/doctest.h>

#include "platform.h"
#include "resizable_buffer.h"
#include "utils.h"

// TODO: figure out a logging solution
//#define MESSAGE_QUEUE_LOG

namespace {

const int kMinimumPartialPayloadSize = 128;

struct MessageHeader {
  MessageHeader(uint32_t partial_id, bool has_more_chunks, size_t size)
      : partial_id(partial_id), has_more_chunks(has_more_chunks), size(size) {}

  uint32_t partial_id;
  bool has_more_chunks;
  size_t size;
};

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
  uint8_t* buffer;
  // Number of bytes left in buffer to parse.
  size_t remaining_bytes;

  BufferMessageIterator(void* buffer, size_t remaining_bytes)
      : buffer(reinterpret_cast<uint8_t*>(buffer)),
        remaining_bytes(remaining_bytes) {}

  MessageHeader* get() const {
    return reinterpret_cast<MessageHeader*>(buffer);
  }
  MessageHeader* operator*() const { return get(); }
  MessageHeader* operator->() const { return get(); }

  void operator++() {
    size_t next_message_offset = sizeof(MessageHeader) + get()->size;
    if (next_message_offset >= remaining_bytes) {
      assert(next_message_offset == remaining_bytes);
      buffer = nullptr;
      remaining_bytes = 0;
      return;
    }

    buffer = buffer + next_message_offset;
    remaining_bytes -= next_message_offset;
  }

  void* message_data() const {
    return reinterpret_cast<void*>(buffer + sizeof(MessageHeader));
  }

  bool operator==(const BufferMessageIterator& other) const {
    return buffer == other.buffer && remaining_bytes == other.remaining_bytes;
  }
  bool operator!=(const BufferMessageIterator& other) const {
    return !(*this == other);
  }
};

enum class RepeatResult { RunAgain, Break };

// Run |action| an arbitrary number of times.
void Repeat(std::function<RepeatResult()> action) {
  bool first = true;
#if defined(MESSAGE_QUEUE_LOG)
  int log_iteration_count = 0;
  int log_count = 0;
#endif
  while (true) {
    if (!first) {
#if defined(MESSAGE_QUEUE_LOG)
      if (log_iteration_count > 1000) {
        log_iteration_count = 0;
        std::cerr << "[info]: Buffer full, waiting (" << log_count++ << ")"
                  << std::endl;
      }
      ++log_iteration_count;
#endif
      // TODO: See if we can figure out a way to use condition variables
      // cross-process.
      std::this_thread::sleep_for(std::chrono::microseconds(0));
    }
    first = false;

    if (action() == RepeatResult::RunAgain)
      continue;
    break;
  }
}

ResizableBuffer* CreateOrFindResizableBuffer(
    std::unordered_map<uint32_t, std::unique_ptr<ResizableBuffer>>&
        resizable_buffers,
    uint32_t id) {
  auto it = resizable_buffers.find(id);
  if (it != resizable_buffers.end())
    return it->second.get();
  return (resizable_buffers[id] = MakeUnique<ResizableBuffer>()).get();
}

std::unique_ptr<Buffer> MakeBuffer(void* content, size_t size) {
  auto buffer = Buffer::Create(size);
  memcpy(buffer->data, content, size);
  return buffer;
}

}  // namespace

Message::Message(void* data, size_t size) : data(data), size(size) {}

struct MessageQueue::BufferMetadata {
  // Reset buffer.
  void reset() { total_message_bytes_ = 0; }

  // Total number of used bytes excluding the sizeof this metadata object.
  void add_used_bytes(size_t used_bytes) { total_message_bytes_ += used_bytes; }

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
  assert(buffer_->capacity >=
         (sizeof(BufferMetadata) + kMinimumPartialPayloadSize));

  if (!buffer_has_data)
    new (buffer_->data) BufferMetadata();

  local_buffer_ = Buffer::Create(buffer_->capacity - sizeof(BufferMetadata));
  memset(local_buffer_->data, 0, local_buffer_->capacity);
}

void MessageQueue::Enqueue(const Message& message) {
#if defined(MESSAGE_QUEUE_LOG)
  int count = 0;
#endif
  uint32_t partial_id = 0;
  uint8_t* payload_data = reinterpret_cast<uint8_t*>(message.data);
  size_t payload_size = message.size;

  Repeat([&]() {
#if defined(MESSAGE_QUEUE_LOG)
    if (count++ > 500) {
      std::cerr << "x500 Sending partial message payload_size=" << payload_size
                << std::endl;
      count = 0;
    }
#endif

    auto lock = buffer_->WaitForExclusiveAccess();

    // We cannot find the entire payload in the buffer. We have to send chunks
    // of it over time.
    if (payload_size >= BytesAvailableInBuffer()) {
      // There's not enough room for our minimum payload size, so try again
      // later.
      if ((sizeof(MessageHeader) + kMinimumPartialPayloadSize) >
          BytesAvailableInBuffer())
        return RepeatResult::RunAgain;

      if (partial_id == 0) {
        // note: pre-increment so we use 1 as the initial value
        partial_id = ++metadata()->next_partial_message_id;
      }

      size_t sent_payload_size =
          BytesAvailableInBuffer() - sizeof(MessageHeader);
      // |sent_payload_size| must always be smaller than |payload_size|. If it
      // is equal to |payload_size|, than we could have sent it as a normal,
      // non-partial message. It's also an error if it is larger than
      // payload_size (we're sending garbage data).
      assert(sent_payload_size < payload_size);

      CopyPayloadToBuffer(partial_id, payload_data, sent_payload_size,
                          true /*has_more_chunks*/);
      payload_data += sent_payload_size;
      payload_size -= sent_payload_size;

      // Prepare for next time.
      return RepeatResult::RunAgain;
    }

    // The entire payload fits. Send it all now.
    else {
      // Include partial message id, as there could have been previous parts of
      // this payload.
      CopyPayloadToBuffer(partial_id, payload_data, payload_size,
                          false /*has_more_chunks*/);

#if defined(MESSAGE_QUEUE_LOG)
      std::cerr << "Sending full message with payload_size=" << payload_size
                << std::endl;
#endif
      return RepeatResult::Break;
    }
  });
}

std::vector<std::unique_ptr<Buffer>> MessageQueue::DequeueAll() {
  std::unordered_map<uint32_t, std::unique_ptr<ResizableBuffer>>
      resizable_buffers;

  std::vector<std::unique_ptr<Buffer>> result;

  while (true) {
    size_t local_buffer_size = 0;

    // Move data from shared memory into a local buffer. Do this
    // before parsing the blocks so that other processes can begin
    // posting data as soon as possible.
    {
      std::unique_ptr<ScopedLock> lock = buffer_->WaitForExclusiveAccess();
      assert(BytesAvailableInBuffer() >= 0);

      // note: Do not copy over buffer_ metadata.
      local_buffer_size = metadata()->total_message_bytes();
      memcpy(local_buffer_->data, first_message_in_buffer(), local_buffer_size);

      metadata()->reset();
    }

    // Parse blocks from shared memory.
    for (auto it = BufferMessageIterator::Begin(local_buffer_->data,
                                                local_buffer_size);
         it != BufferMessageIterator::End(); ++it) {
#if defined(MESSAGE_QUEUE_LOG)
      std::cerr << "Got message with partial_id=" << it->partial_id
                << ", payload_size=" << it->size
                << ", has_more_chunks=" << it->has_more_chunks << std::endl;
#endif

      if (it->partial_id != 0) {
        auto* buf =
            CreateOrFindResizableBuffer(resizable_buffers, it->partial_id);
        buf->Append(it.message_data(), it->size);

        if (!it->has_more_chunks) {
          result.push_back(MakeBuffer(buf->buffer, buf->size));
          resizable_buffers.erase(it->partial_id);
        }
      } else {
        // Note: we can't just return pointers to |local_buffer_| because if we
        // read a partial message we will invalidate all of the existing
        // pointers. We could jump through hoops to make it work (ie, if no
        // partial messages return pointers to local_buffer_) but it is not
        // worth the effort.
        assert(!it->has_more_chunks);
        result.push_back(MakeBuffer(it.message_data(), it->size));
      }
    }

    // We're waiting for data to be posted to result. Delay a little so we
    // don't push the CPU so hard.
    if (!resizable_buffers.empty())
      std::this_thread::sleep_for(std::chrono::microseconds(0));
    else
      break;
  }

  return result;
}

void MessageQueue::CopyPayloadToBuffer(uint32_t partial_id,
                                       void* payload,
                                       size_t payload_size,
                                       bool has_more_chunks) {
  assert(BytesAvailableInBuffer() >= (sizeof(MessageHeader) + payload_size));

  // Copy header.
  MessageHeader header(partial_id, has_more_chunks, payload_size);
  memcpy(first_free_address_in_buffer(), &header, sizeof(MessageHeader));
  metadata()->add_used_bytes(sizeof(MessageHeader));
  // Copy payload.
  memcpy(first_free_address_in_buffer(), payload, payload_size);
  metadata()->add_used_bytes(payload_size);
}

MessageQueue::BufferMetadata* MessageQueue::metadata() const {
  return reinterpret_cast<BufferMetadata*>(buffer_->data);
}

size_t MessageQueue::BytesAvailableInBuffer() const {
  return buffer_->capacity - metadata()->total_bytes_used_including_metadata();
}

Message* MessageQueue::first_message_in_buffer() const {
  return reinterpret_cast<Message*>(reinterpret_cast<uint8_t*>(buffer_->data) +
                                    sizeof(BufferMetadata));
}

void* MessageQueue::first_free_address_in_buffer() const {
  if (metadata()->total_bytes_used_including_metadata() >= buffer_->capacity)
    return nullptr;
  return reinterpret_cast<void*>(
      reinterpret_cast<uint8_t*>(buffer_->data) +
      metadata()->total_bytes_used_including_metadata());
}

TEST_SUITE("MessageQueue");

TEST_CASE("simple") {
  MessageQueue queue(Buffer::Create(kMinimumPartialPayloadSize * 5),
                     false /*buffer_has_data*/);

  int data = 0;
  data = 1;
  queue.Enqueue(Message(&data, sizeof(data)));
  data = 2;
  queue.Enqueue(Message(&data, sizeof(data)));

  int expected = 0;
  for (std::unique_ptr<Buffer>& m : queue.DequeueAll()) {
    ++expected;

    REQUIRE(m->capacity == sizeof(data));
    int* value = reinterpret_cast<int*>(m->data);
    REQUIRE(expected == *value);
  }
}

TEST_CASE("large payload") {
  MessageQueue queue(Buffer::Create(kMinimumPartialPayloadSize * 5),
                     false /*buffer_has_data*/);

  // Allocate big buffer.
  size_t num_ints = kMinimumPartialPayloadSize * 100;
  int* sent_ints = reinterpret_cast<int*>(malloc(sizeof(int) * num_ints));
  for (int i = 0; i < num_ints; ++i)
    sent_ints[i] = i;

  // Queue big buffer. Add surrounding messages to make sure they get sent
  // correctly.
  // Run in a separate thread because Enqueue will block.
  volatile bool done_sending = false;
  std::thread sender([&]() {
    int small = 5;
    queue.Enqueue(Message(&small, sizeof(small)));
    queue.Enqueue(Message(sent_ints, sizeof(int) * num_ints));
    queue.Enqueue(Message(&small, sizeof(small)));
    done_sending = true;
  });

  // Receive sent messages.
  {
    // Keep dequeuing messages until we have three.
    std::vector<std::unique_ptr<Buffer>> messages;
    while (messages.size() != 3) {
      for (auto& message : queue.DequeueAll())
        messages.emplace_back(std::move(message));
    }
    sender.join();

    // Small
    {
      REQUIRE(sizeof(int) == messages[0]->capacity);
      int* value = reinterpret_cast<int*>(messages[0]->data);
      REQUIRE(*value == 5);
    }

    // Big
    {
      int* received_ints = reinterpret_cast<int*>(messages[1]->data);
      REQUIRE(received_ints != sent_ints);
      REQUIRE(messages[1]->capacity == (sizeof(int) * num_ints));
      for (int i = 0; i < num_ints; ++i) {
        REQUIRE(received_ints[i] == i);
        REQUIRE(received_ints[i] == sent_ints[i]);
      }
    }

    // Small
    {
      REQUIRE(sizeof(int) == messages[2]->capacity);
      int* value = reinterpret_cast<int*>(messages[2]->data);
      REQUIRE(*value == 5);
    }
  }

  free(sent_ints);
}

TEST_SUITE_END();
