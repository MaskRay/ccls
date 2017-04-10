#pragma once

#include <memory>
#include <string>

struct ScopedLock {
  virtual ~ScopedLock() = default;
};

// Points to a generic block of memory. Note that |data| is relocatable, ie,
// multiple Buffer instantiations may point to the same underlying block of
// memory but the data pointer has different values.
struct Buffer {
  // Create a new buffer of the given capacity using process-local memory.
  static std::unique_ptr<Buffer> Create(size_t capacity);
  // Create a buffer pointing to memory shared across processes with the given
  // capacity.
  static std::unique_ptr<Buffer> CreateSharedBuffer(const std::string& name,
                                                    size_t capacity);

  virtual ~Buffer() = default;

  // Acquire a lock on the buffer, ie, become the only code that can read or
  // write to it. The lock lasts so long as the returned object is alive.
  virtual std::unique_ptr<ScopedLock> WaitForExclusiveAccess() = 0;

  void* data = nullptr;
  size_t capacity = 0;
};
