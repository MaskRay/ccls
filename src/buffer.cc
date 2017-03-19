#include "buffer.h"

#include <mutex>

#include "platform.h"
#include "../utils.h"
#include "../third_party/doctest/doctest/doctest.h"

namespace {

struct ScopedLockLocal : public ScopedLock {
  ScopedLockLocal(std::mutex& mutex) : guard(mutex) {}
  std::lock_guard<std::mutex> guard;
};

struct BufferLocal : public Buffer {
  explicit BufferLocal(size_t capacity) {
    this->data = malloc(capacity);
    this->capacity = capacity;
  }
  ~BufferLocal() override {
    free(data);
    data = nullptr;
    capacity = 0;
  }

  std::unique_ptr<ScopedLock> WaitForExclusiveAccess() override {
    return MakeUnique<ScopedLockLocal>(mutex_);
  }

  std::mutex mutex_;
};

struct ScopedLockPlatform : public ScopedLock {
  ScopedLockPlatform(PlatformMutex* mutex)
    : guard(CreatePlatformScopedMutexLock(mutex)) {}

  std::unique_ptr<PlatformScopedMutexLock> guard;
};

struct BufferPlatform : public Buffer {
  explicit BufferPlatform(const std::string& name, size_t capacity)
    : memory_(CreatePlatformSharedMemory(name + "_mem", capacity)),
      mutex_(CreatePlatformMutex(name + "_mtx")) {
    this->data = memory_->data;
    this->capacity = memory_->capacity;
  }

  ~BufferPlatform() override {
    data = nullptr;
    capacity = 0;
  }

  std::unique_ptr<ScopedLock> WaitForExclusiveAccess() override {
    return MakeUnique<ScopedLockPlatform>(mutex_.get());
  }

  std::unique_ptr<PlatformSharedMemory> memory_;
  std::unique_ptr<PlatformMutex> mutex_;
};

}  // namespace

std::unique_ptr<Buffer> Buffer::Create(size_t capacity) {
  return MakeUnique<BufferLocal>(capacity);
}

std::unique_ptr<Buffer> Buffer::CreateSharedBuffer(const std::string& name, size_t capacity) {
  return MakeUnique<BufferPlatform>(name, capacity);
}

TEST_SUITE("BufferLocal");

TEST_CASE("create") {
  std::unique_ptr<Buffer> b = Buffer::Create(24);
  REQUIRE(b->data);
  REQUIRE(b->capacity == 24);

  b = Buffer::CreateSharedBuffer("indexertest", 24);
  REQUIRE(b->data);
  REQUIRE(b->capacity == 24);
}

TEST_CASE("lock") {
  auto buffers = {
    Buffer::Create(sizeof(int)),
    Buffer::CreateSharedBuffer("indexertest", sizeof(int))
  };

  for (auto& b : buffers) {
    int* data = reinterpret_cast<int*>(b->data);
    *data = 0;

    std::unique_ptr<std::thread> thread;
    {
      auto lock = b->WaitForExclusiveAccess();
      *data = 1;

      // Start a second thread, wait until it has attempted to acquire a lock.
      volatile bool did_read = false;
      thread = MakeUnique<std::thread>([&did_read, &b, &data]() {
        did_read = true;
        auto l = b->WaitForExclusiveAccess();
        *data = 2;
      });
      while (!did_read)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

      // Verify lock acquisition is waiting.
      REQUIRE(*data == 1);
    }

    // Wait for thread to acquire lock, verify it writes to data.
    thread->join();
    REQUIRE(*data == 2);
  }
}

TEST_SUITE_END();