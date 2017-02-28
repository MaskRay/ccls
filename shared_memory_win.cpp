#include <iostream>

#include <Windows.h>

const int shmem_size = 16;  // 16byte

void reader() {
  HANDLE shmem = INVALID_HANDLE_VALUE;
  HANDLE mutex = INVALID_HANDLE_VALUE;

  mutex = ::CreateMutex(NULL, FALSE, "mutex_sample_name");

  shmem = ::CreateFileMapping(
    INVALID_HANDLE_VALUE,
    NULL,
    PAGE_READWRITE,
    0,
    shmem_size,
    "shared_memory_name"
  );

  char *buf = (char*)MapViewOfFile(shmem, FILE_MAP_ALL_ACCESS, 0, 0, shmem_size);


  for (unsigned int c = 0; c < 60; ++c) {
    // mutex lock
    WaitForSingleObject(mutex, INFINITE);

    int value = buf[0];
    std::cout << "read shared memory...c=" << value << std::endl;

    // mutex unlock
    ::ReleaseMutex(mutex);

    ::Sleep(1000);
  }

  // release
  ::UnmapViewOfFile(buf);
  ::CloseHandle(shmem);
  ::ReleaseMutex(mutex);
}

void writer() {
  HANDLE	shmem = INVALID_HANDLE_VALUE;
  HANDLE	mutex = INVALID_HANDLE_VALUE;

  mutex = ::CreateMutex(NULL, FALSE, "mutex_sample_name");

  shmem = ::CreateFileMapping(
    INVALID_HANDLE_VALUE,
    NULL,
    PAGE_READWRITE,
    0,
    shmem_size,
    "shared_memory_name"
  );

  char *buf = (char*)::MapViewOfFile(shmem, FILE_MAP_ALL_ACCESS, 0, 0, shmem_size);

  for (unsigned int c = 0; c < 60; ++c) {
    // mutex lock
    WaitForSingleObject(mutex, INFINITE);

    // write shared memory
    memset(buf, c, shmem_size);

    std::cout << "write shared memory...c=" << c << std::endl;

    // mutex unlock
    ::ReleaseMutex(mutex);

    ::Sleep(1000);
  }

  // release
  ::UnmapViewOfFile(buf);
  ::CloseHandle(shmem);
  ::ReleaseMutex(mutex);
}

int main52525252(int argc, char** argv) {
  if (argc == 2)
    writer();
  else
    reader();

  return 0;
}