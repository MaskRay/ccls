#pragma once

#include <memory>
#include <string>
#include <vector>

struct PlatformMutex {
  virtual ~PlatformMutex();
};
struct PlatformScopedMutexLock {
  virtual ~PlatformScopedMutexLock();
};
struct PlatformSharedMemory {
  virtual ~PlatformSharedMemory();
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

void PlatformInit();
std::string GetWorkingDirectory();
std::string NormalizePath(const std::string& path);

// Returns any clang arguments that are specific to the current platform.
std::vector<std::string> GetPlatformClangArguments();