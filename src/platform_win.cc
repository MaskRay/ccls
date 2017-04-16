#if defined(_WIN32)
#include "platform.h"

#include "utils.h"

#include <fcntl.h>
#include <io.h>
#include <Windows.h>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>

namespace {

DWORD CheckForError(std::vector<DWORD> allow) {
  DWORD error = GetLastError();
  if (error == ERROR_SUCCESS ||
    std::find(allow.begin(), allow.end(), error) != allow.end())
    return error;

  // See http://stackoverflow.com/a/17387176
  LPSTR message_buffer = nullptr;
  size_t size = FormatMessageA(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPSTR)&message_buffer, 0, NULL);
  std::string message(message_buffer, size);
  LocalFree(message_buffer);

  std::cerr << "Windows error code=" << error << ", message=" << message
    << std::endl;

  assert(false);  // debugger break
  exit(1);
}

struct PlatformMutexWin : public PlatformMutex {
  HANDLE raw_mutex = INVALID_HANDLE_VALUE;

  PlatformMutexWin(const std::string& name) {
    std::cerr << "[win] Creating mutex with name " << name << std::endl;
    raw_mutex = CreateMutex(nullptr, false /*initial_owner*/, name.c_str());
    CheckForError({ ERROR_ALREADY_EXISTS });
  }

  ~PlatformMutexWin() override {
    CloseHandle(raw_mutex);
    CheckForError({} /*allow*/);
    raw_mutex = INVALID_HANDLE_VALUE;
  }
};

struct PlatformScopedMutexLockWin : public PlatformScopedMutexLock {
  HANDLE raw_mutex;

  PlatformScopedMutexLockWin(HANDLE raw_mutex) : raw_mutex(raw_mutex) {
    DWORD result = WaitForSingleObject(raw_mutex, INFINITE);
    assert(result == WAIT_OBJECT_0);
    CheckForError({ ERROR_NO_MORE_FILES, ERROR_ALREADY_EXISTS } /*allow*/);
  }

  ~PlatformScopedMutexLockWin() override {
    ReleaseMutex(raw_mutex);
    CheckForError({ ERROR_NO_MORE_FILES, ERROR_ALREADY_EXISTS } /*allow*/);
  }
};

struct PlatformSharedMemoryWin : public PlatformSharedMemory {
  HANDLE shmem_;

  PlatformSharedMemoryWin(const std::string& name, size_t capacity) {
    std::cerr << "[win] Creating shared memory with name " << name
      << " and capacity " << capacity << std::endl;
    this->name = name;

    shmem_ = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
      capacity, name.c_str());
    CheckForError({ ERROR_ALREADY_EXISTS } /*allow*/);

    data = MapViewOfFile(shmem_, FILE_MAP_ALL_ACCESS, 0, 0, capacity);
    CheckForError({ ERROR_ALREADY_EXISTS } /*allow*/);

    this->capacity = capacity;
  }

  ~PlatformSharedMemoryWin() override {
    UnmapViewOfFile(data);
    CheckForError({} /*allow*/);

    data = nullptr;
    capacity = 0;
  }
};

}  // namespace

std::unique_ptr<PlatformMutex> CreatePlatformMutex(const std::string& name) {
  return MakeUnique<PlatformMutexWin>(name);
}

std::unique_ptr<PlatformScopedMutexLock> CreatePlatformScopedMutexLock(
  PlatformMutex* mutex) {
  return MakeUnique<PlatformScopedMutexLockWin>(
    static_cast<PlatformMutexWin*>(mutex)->raw_mutex);
}

std::unique_ptr<PlatformSharedMemory> CreatePlatformSharedMemory(
  const std::string& name,
  size_t size) {
  return MakeUnique<PlatformSharedMemoryWin>(name, size);
}

void PlatformInit() {
  // We need to write to stdout in binary mode because in Windows, writing
  // \n will implicitly write \r\n. Language server API will ignore a
  // \r\r\n split request.
  _setmode(_fileno(stdout), O_BINARY);
  _setmode(_fileno(stdin), O_BINARY);
}

// See http://stackoverflow.com/a/19535628
std::string GetWorkingDirectory() {
  char result[MAX_PATH];
  return std::string(result, GetModuleFileName(NULL, result, MAX_PATH));
}

std::string NormalizePath(const std::string& path) {
  DWORD retval = 0;
  BOOL success = false;
  TCHAR buffer[MAX_PATH] = TEXT("");
  TCHAR buf[MAX_PATH] = TEXT("");
  TCHAR** lpp_part = { NULL };

  retval = GetFullPathName(path.c_str(), MAX_PATH, buffer, lpp_part);
  // fail, return original
  if (retval == 0)
    return path;

  std::string result = buffer;
  std::replace(result.begin(), result.end(), '\\', '/');
  //std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}

std::vector<std::string> GetPlatformClangArguments() {
  return {
    "-fms-compatibility",
    "-fdelayed-template-parsing"
  };
}
#endif
