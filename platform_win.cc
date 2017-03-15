#ifdef _MSC_VER
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

  PlatformSharedMemoryWin(const std::string& name) {
    this->name = name;

    shmem_ = CreateFileMapping(
      INVALID_HANDLE_VALUE,
      NULL,
      PAGE_READWRITE,
      0,
      shmem_size,
      name.c_str()
    );

    shared = MapViewOfFile(shmem_, FILE_MAP_ALL_ACCESS, 0, 0, shmem_size);
  }

  ~PlatformSharedMemoryWin() override {
    UnmapViewOfFile(shared);
    shared = nullptr;
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

// See http://stackoverflow.com/a/19535628
std::string GetWorkingDirectory() {
  char result[MAX_PATH];
  return std::string(result, GetModuleFileName(NULL, result, MAX_PATH));
}

/*
// linux
#include <string>
#include <limits.h>
#include <unistd.h>

std::string getexepath() {
  char result[ PATH_MAX ];
  ssize_t count = readlink( "/proc/self/exe", result, PATH_MAX );
  return std::string( result, (count > 0) ? count : 0 );
}
*/
#endif
