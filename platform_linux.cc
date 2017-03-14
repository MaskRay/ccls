#include "platform.h"

#include "utils.h"

#include <cassert>
#include <string>
#include <pthread.h>
#include <iostream>



#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>

#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

struct PlatformMutexLinux : public PlatformMutex {
  sem_t* sem_ = nullptr;

  PlatformMutexLinux(const std::string& name) {
    std::cout << "PlatformMutexLinux name=" << name << std::endl;
    sem_ = sem_open(name.c_str(), O_CREAT, 0666 /*permission*/, 1 /*initial_value*/);
  }

  ~PlatformMutexLinux() override {
    sem_close(sem_);
  }
};

struct PlatformScopedMutexLockLinux : public PlatformScopedMutexLock {
  sem_t* sem_ = nullptr;

  PlatformScopedMutexLockLinux(sem_t* sem) : sem_(sem) {
    std::cout << "PlatformScopedMutexLockLinux" << std::endl;
    sem_wait(sem_);
  }

  ~PlatformScopedMutexLockLinux() override {
    sem_post(sem_);
  }
};

struct PlatformSharedMemoryLinux : public PlatformSharedMemory {
  std::string name_;
  int fd_;

  PlatformSharedMemoryLinux(const std::string& name) : name_(name) {
    std::cout << "PlatformSharedMemoryLinux name=" << name << std::endl;
    fd_ = shm_open(name_.c_str(), O_CREAT, O_RDWR);
    std::cout << "shm_open errno=" << errno << std::endl;
    std::cout << "1" << std::endl;
    ftruncate(fd_, shmem_size);
    std::cout << "ftruncate errno=" << errno << std::endl;
    std::cout << "2" << std::endl;

   //void *mmap(void *addr, size_t length, int prot, int flags,
   //           int fd, off_t offset);
   //int munmap(void *addr, size_t length);
   //
    //shmem_size
    std::cout << "3" << std::endl;
    shared = mmap(nullptr /*kernel assigned starting address*/,
         shmem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0 /*offset*/);
    std::cout << "mmap errno=" << errno << std::endl;
    std::cout << "fd_ = " << fd_ << std::endl;
    std::cout << "shared = " << shared << std::endl;
    std::cout << "4" << std::endl;

    /*
      int fd = shm_open("shmname", O_CREAT, O_RDWR);
      sem_t *sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
      ftruncate(fd_, shmem_size);

      sem_init(sem, 1, 1);
     */
  }

  ~PlatformSharedMemoryLinux() override {
    munmap(shared, shmem_size);
    shared = nullptr;
    shm_unlink(name_.c_str());
  }
};



std::unique_ptr<PlatformMutex> CreatePlatformMutex(const std::string& name) {
  std::string name2 = "/" + name;
  return MakeUnique<PlatformMutexLinux>(name2);
}

std::unique_ptr<PlatformScopedMutexLock> CreatePlatformScopedMutexLock(PlatformMutex* mutex) {
  return MakeUnique<PlatformScopedMutexLockLinux>(static_cast<PlatformMutexLinux*>(mutex)->sem_);
}

std::unique_ptr<PlatformSharedMemory> CreatePlatformSharedMemory(const std::string& name) {
  std::string name2 = "/" + name;
  return MakeUnique<PlatformSharedMemoryLinux>(name2);
}
