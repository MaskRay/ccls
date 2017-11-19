#if defined(__linux__) || defined(__APPLE__)
#include "platform.h"

#include "utils.h"

#include <loguru.hpp>

#include <pthread.h>
#include <cassert>
#include <iostream>
#include <string>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>  // required for stat.h

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <fcntl.h> /* For O_* constants */
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */

#ifndef __APPLE__
#include <sys/prctl.h>
#endif

#if defined(__linux__)
#include <malloc.h>
#endif

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
    std::cerr << "PlatformSharedMemoryLinux name=" << name << ", size=" << size
              << std::endl;

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
    data = CHECKED(mmap(nullptr /*kernel assigned starting address*/, size,
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
    const std::string& name,
    size_t size) {
  std::string name2 = "/" + name;
  return MakeUnique<PlatformSharedMemoryLinux>(name2, size);
}

void PlatformInit() {}

std::string GetWorkingDirectory() {
  char result[FILENAME_MAX];
  if (!getcwd(result, sizeof(result)))
    return "";
  std::string working_dir = std::string(result, strlen(result));
  EnsureEndsInSlash(working_dir);
  return working_dir;
}

std::string NormalizePath(const std::string& path) {
  errno = 0;
  char name[PATH_MAX + 1];
  (void)realpath(path.c_str(), name);
  if (errno)
    return path;
  return name;
}

bool TryMakeDirectory(const std::string& absolute_path) {
  const mode_t kMode = 0777;  // UNIX style permissions
  if (mkdir(absolute_path.c_str(), kMode) == -1) {
    // Success if the directory exists.
    return errno == EEXIST;
  }
  return true;
}

void SetCurrentThreadName(const std::string& thread_name) {
  loguru::set_thread_name(thread_name.c_str());
#ifndef __APPLE__
  prctl(PR_SET_NAME, thread_name.c_str(), 0, 0, 0);
#endif
}

optional<int64_t> GetLastModificationTime(const std::string& absolute_path) {
  struct stat buf;
  if (stat(absolute_path.c_str(), &buf) != 0) {
    switch (errno) {
      case ENOENT:
        // std::cerr << "GetLastModificationTime: unable to find file " <<
        // absolute_path << std::endl;
        return nullopt;
      case EINVAL:
        // std::cerr << "GetLastModificationTime: invalid param to _stat for
        // file file " << absolute_path << std::endl;
        return nullopt;
      default:
        // std::cerr << "GetLastModificationTime: unhandled for " <<
        // absolute_path << std::endl;  exit(1);
        return nullopt;
    }
  }

  return buf.st_mtime;
}

void MoveFileTo(const std::string& dest, const std::string& source) {
  // TODO/FIXME - do a real move.
  CopyFileTo(dest, source);
}

// See http://stackoverflow.com/q/13198627
void CopyFileTo(const std::string& dest, const std::string& source) {
  int fd_from = open(source.c_str(), O_RDONLY);
  if (fd_from < 0)
    return;

  int fd_to = open(dest.c_str(), O_WRONLY | O_CREAT, 0666);
  if (fd_to < 0)
    goto out_error;

  char buf[4096];
  ssize_t nread;
  while (nread = read(fd_from, buf, sizeof buf), nread > 0) {
    char* out_ptr = buf;
    ssize_t nwritten;

    do {
      nwritten = write(fd_to, out_ptr, nread);

      if (nwritten >= 0) {
        nread -= nwritten;
        out_ptr += nwritten;
      } else if (errno != EINTR)
        goto out_error;
    } while (nread > 0);
  }

  if (nread == 0) {
    if (close(fd_to) < 0) {
      fd_to = -1;
      goto out_error;
    }
    close(fd_from);

    return;
  }

out_error:
  close(fd_from);
  if (fd_to >= 0)
    close(fd_to);
}

bool IsSymLink(const std::string& path) {
  struct stat buf;
  int result;
  result = lstat(path.c_str(), &buf);
  return result == 0;
}

std::vector<std::string> GetPlatformClangArguments() {
  return {};
}
#undef CHECKED

void FreeUnusedMemory() {
#if defined(__linux__)
  malloc_trim(0);
#endif
}

#endif
