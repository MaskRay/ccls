#if defined(__linux__) || defined(__APPLE__)
#include "platform.h"

#include "utils.h"

#include <cassert>
#include <string>
#include <pthread.h>
#include <iostream>

#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>

#include <fcntl.h>    /* For O_* constants */
#include <sys/stat.h> /* For mode constants */
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h>    /* For O_* constants */

struct PlatformMutexLinux : public PlatformMutex {
  sem_t* sem_ = nullptr;

  PlatformMutexLinux(const std::string& name) {
    std::cerr << "PlatformMutexLinux name=" << name << std::endl;
    sem_ = sem_open(name.c_str(), O_CREAT, 0666 /*permission*/,
                    1 /*initial_value*/);
  }

  ~PlatformMutexLinux() override { sem_close(sem_); }
};

struct PlatformScopedMutexLockLinux : public PlatformScopedMutexLock {
  sem_t* sem_ = nullptr;

  PlatformScopedMutexLockLinux(sem_t* sem) : sem_(sem) { sem_wait(sem_); }

  ~PlatformScopedMutexLockLinux() override { sem_post(sem_); }
};

void* checked(void* result, const char* expr) {
  if (!result) {
    std::cerr << "FAIL errno=" << errno << " in |" << expr << "|" << std::endl;
    std::cerr << "errno => " << strerror(errno) << std::endl;
    exit(1);
  }
  return result;
}

int checked(int result, const char* expr) {
  if (result == -1) {
    std::cerr << "FAIL errno=" << errno << " in |" << expr << "|" << std::endl;
    std::cerr << "errno => " << strerror(errno) << std::endl;
    exit(1);
  }
  return result;
}

#define CHECKED(expr) checked(expr, #expr)

struct PlatformSharedMemoryLinux : public PlatformSharedMemory {
  std::string name_;
  size_t size_;
  int fd_;

  PlatformSharedMemoryLinux(const std::string& name, size_t size)
    : name_(name), size_(size) {
    std::cerr << "PlatformSharedMemoryLinux name=" << name << ", size=" << size << std::endl;

    // Try to create shared memory but only if it does not already exist. Since
    // we created the memory, we need to initialize it.
    fd_ = shm_open(name_.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd_ >= 0) {
      std::cerr << "Calling ftruncate fd_=" << fd_ << std::endl;
      CHECKED(ftruncate(fd_, size));
    }

    // Otherwise, we just open existing shared memory. We don't need to
    // create or initialize it.
    else {
      fd_ = CHECKED(shm_open(
          name_.c_str(), O_RDWR, /* memory is read/write, create if needed */
          S_IRUSR | S_IWUSR /* user read/write */));
    }

    // Map the shared memory to an address.
    data =
        CHECKED(mmap(nullptr /*kernel assigned starting address*/, size,
                     PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0 /*offset*/));
    capacity = size;

    std::cerr << "Open shared memory name=" << name << ", fd=" << fd_
              << ", shared=" << data << ", capacity=" << capacity << std::endl;
  }

  ~PlatformSharedMemoryLinux() override {
    CHECKED(munmap(data, size_));
    CHECKED(shm_unlink(name_.c_str()));

    data = nullptr;
  }
};

std::unique_ptr<PlatformMutex> CreatePlatformMutex(const std::string& name) {
  std::string name2 = "/" + name;
  return MakeUnique<PlatformMutexLinux>(name2);
}

std::unique_ptr<PlatformScopedMutexLock> CreatePlatformScopedMutexLock(
    PlatformMutex* mutex) {
  return MakeUnique<PlatformScopedMutexLockLinux>(
      static_cast<PlatformMutexLinux*>(mutex)->sem_);
}

std::unique_ptr<PlatformSharedMemory> CreatePlatformSharedMemory(
    const std::string& name, size_t size) {
  std::string name2 = "/" + name;
  return MakeUnique<PlatformSharedMemoryLinux>(name2, size);
}

void PlatformInit() {
}

std::string NormalizePath(const std::string& path) {
  errno = 0;
  char name[PATH_MAX + 1];
  realpath(path.c_str(), name);
  if (errno)
    return path;
  return name;
}

std::vector<std::string> GetPlatformClangArguments() {
  // TODO: use install config variable for path?
  return {
    "-stdlib=libc++",
    "-nostdinc++",
    //"-I/usr/local/Cellar/llvm/3.9.1/include",
    "-I/Users/jdufault/Personal/super-clang-index/libcxx/include"
  };
}
#undef CHECKED

#endif
