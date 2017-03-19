#pragma once

// Points to a generic block of memory that can be resized. This class owns
// and has the only pointer to the underlying memory buffer.
struct ResizableBuffer {
  ResizableBuffer();
  ResizableBuffer(const ResizableBuffer&) = delete;
  ~ResizableBuffer();

  void Append(void* content, size_t content_size);
  void Reset();

  // Buffer content.
  void* buffer;
  // Number of bytes in |buffer|. Note that the actual buffer may be larger
  // than |size|.
  size_t size;

private:
  size_t capacity_;
};