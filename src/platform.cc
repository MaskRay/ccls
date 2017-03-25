#include "platform.h"

#include <thread>

#include <doctest/doctest.h>

PlatformMutex::~PlatformMutex() = default;

PlatformScopedMutexLock::~PlatformScopedMutexLock() = default;

PlatformSharedMemory::~PlatformSharedMemory() = default;

TEST_SUITE("Platform");

TEST_CASE("Mutex lock/unlock (single process)") {
  auto m1 = CreatePlatformMutex("indexer-platformmutexttest");
  auto l1 = CreatePlatformScopedMutexLock(m1.get());
  auto m2 = CreatePlatformMutex("indexer-platformmutexttest");

  int value = 0;

  volatile bool did_run = false;
  std::thread t([&]() {
    did_run = true;
    auto l2 = CreatePlatformScopedMutexLock(m2.get());
    value = 1;
  });
  while (!did_run)
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  // Other thread has had a chance to run, but it should not have
  // written to value yet (ie, it should be waiting).
  REQUIRE(value == 0);

  // Release the lock, wait for other thread to finish. Verify it
  // wrote the expected value.
  l1.reset();
  t.join();
  REQUIRE(value == 1);
}

TEST_SUITE_END();
