#pragma once

#include <memory>
#include <string>

struct PlatformMutex {
  virtual ~PlatformMutex() {}
};
struct PlatformScopedMutexLock {
  virtual ~PlatformScopedMutexLock() {}
};
struct PlatformSharedMemory {
  virtual ~PlatformSharedMemory() {}
  void* shared;
};

const int shmem_size = 200;// 1024 * 1024 * 32;  // number of chars/bytes (32mb)

std::unique_ptr<PlatformMutex> CreatePlatformMutex(const std::string& name);
std::unique_ptr<PlatformScopedMutexLock> CreatePlatformScopedMutexLock(PlatformMutex* mutex);
std::unique_ptr<PlatformSharedMemory> CreatePlatformSharedMemory(const std::string& name);

std::string GetWorkingDirectory();