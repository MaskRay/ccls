#include "resizable_buffer.h"

#include "../third_party/doctest/doctest/doctest.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {
const size_t kInitialCapacity = 128;
}

ResizableBuffer::ResizableBuffer() {
  buffer = malloc(kInitialCapacity);
  size = 0;
  capacity_ = kInitialCapacity;
}

ResizableBuffer::~ResizableBuffer() {
  free(buffer);
  size = 0;
  capacity_ = 0;
}

void ResizableBuffer::Append(void* content, size_t content_size) {
  assert(capacity_ >= 0);

  size_t new_size = size + content_size;
  
  // Grow buffer capacity if needed.
  if (new_size >= capacity_) {
    size_t new_capacity = capacity_ * 2;
    while (new_size >= new_capacity)
      new_capacity *= 2;
    void* new_memory = malloc(new_capacity);
    assert(size < capacity_);
    memcpy(new_memory, buffer, size);
    free(buffer);
    buffer = new_memory;
    capacity_ = new_capacity;
  }

  // Append new content into memory.
  memcpy(reinterpret_cast<uint8_t*>(buffer) + size, content, content_size);
  size = new_size;
}

void ResizableBuffer::Reset() {
  size = 0;
}

TEST_SUITE("ResizableBuffer");

TEST_CASE("buffer starts with zero size") {
  ResizableBuffer b;
  REQUIRE(b.buffer);
  REQUIRE(b.size == 0);
}

TEST_CASE("append and reset") {
  int content = 1;
  ResizableBuffer b;

  b.Append(&content, sizeof(content));
  REQUIRE(b.size == sizeof(content));

  b.Append(&content, sizeof(content));
  REQUIRE(b.size == (2 * sizeof(content)));

  b.Reset();
  REQUIRE(b.size == 0);
}

TEST_CASE("appended content is copied into buffer w/ resize") {
  int content = 0;
  ResizableBuffer b;

  // go past kInitialCapacity to verify resize works too
  while (b.size < kInitialCapacity * 2) {
    b.Append(&content, sizeof(content));
    content += 1;
  }

  for (int i = 0; i < content; ++i)
    REQUIRE(i == *(reinterpret_cast<int*>(b.buffer) + i));
}

TEST_CASE("reset does not reallocate") {
  ResizableBuffer b;

  while (b.size < kInitialCapacity)
    b.Append(&b, sizeof(b));
  
  void* buffer = b.buffer;
  b.Reset();
  REQUIRE(b.buffer == buffer);
}

TEST_SUITE_END();