#include "platform.h"

#include <cassert>
#include <string>
#include <Windows.h>

#include "utils.h"

struct PlatformMutexWin : public PlatformMutex {
  HANDLE raw_mutex = INVALID_HANDLE_VALUE;

  PlatformMutexWin(const std::string& name) {
    raw_mutex = CreateMutex(nullptr, false /*initial_owner*/, name.c_str());
    assert(GetLastError() != ERROR_INVALID_HANDLE);
  }

  ~PlatformMutexWin() override {
    ReleaseMutex(raw_mutex);
    raw_mutex = INVALID_HANDLE_VALUE;
  }
};

struct PlatformScopedMutexLockWin : public PlatformScopedMutexLock {
  HANDLE raw_mutex;

  PlatformScopedMutexLockWin(HANDLE raw_mutex) : raw_mutex(raw_mutex) {
    WaitForSingleObject(raw_mutex, INFINITE);
  }

  ~PlatformScopedMutexLockWin() override {
    ReleaseMutex(raw_mutex);
  }
};

struct PlatformSharedMemoryWin : public PlatformSharedMemory {
  HANDLE shmem_;
  void* shared_start_real_;

  PlatformSharedMemoryWin(const std::string& name) {
    shmem_ = CreateFileMapping(
      INVALID_HANDLE_VALUE,
      NULL,
      PAGE_READWRITE,
      0,
      shmem_size,
      name.c_str()
    );

    shared_start_real_ = MapViewOfFile(shmem_, FILE_MAP_ALL_ACCESS, 0, 0, shmem_size);

    shared_bytes_used = reinterpret_cast<size_t*>(shared_start_real_);
    *shared_bytes_used = 0;
    shared_start = reinterpret_cast<char*>(shared_bytes_used + 1);
  }

  ~PlatformSharedMemoryWin() override {
    UnmapViewOfFile(shared_start_real_);
  }
};



std::unique_ptr<PlatformMutex> CreatePlatformMutex(const std::string& name) {
  return MakeUnique<PlatformMutexWin>(name);
}

std::unique_ptr<PlatformScopedMutexLock> CreatePlatformScopedMutexLock(PlatformMutex* mutex) {
  return MakeUnique<PlatformScopedMutexLockWin>(static_cast<PlatformMutexWin*>(mutex)->raw_mutex);
}

std::unique_ptr<PlatformSharedMemory> CreatePlatformSharedMemory(const std::string& name) {
  return MakeUnique<PlatformSharedMemoryWin>(name);
}
