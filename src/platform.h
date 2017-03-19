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
  void* data;
  size_t capacity;
  std::string name;
};

std::unique_ptr<PlatformMutex> CreatePlatformMutex(const std::string& name);
std::unique_ptr<PlatformScopedMutexLock> CreatePlatformScopedMutexLock(
    PlatformMutex* mutex);
std::unique_ptr<PlatformSharedMemory> CreatePlatformSharedMemory(
    const std::string& name,
    size_t size);

std::string GetWorkingDirectory();