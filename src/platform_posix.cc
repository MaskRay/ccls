// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#if defined(__unix__) || defined(__APPLE__) || defined(__HAIKU__)
#include "platform.hh"

#include "utils.hh"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h> // required for stat.h
#include <sys/wait.h>
#include <unistd.h>
#ifdef __GLIBC__
#include <malloc.h>
#endif

#include <llvm/ADT/SmallString.h>
#include <llvm/Support/Path.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>

namespace ccls {
namespace pipeline {
void threadEnter();
}

std::string normalizePath(llvm::StringRef path) {
  llvm::SmallString<256> p(path);
  llvm::sys::path::remove_dots(p, true);
  return {p.data(), p.size()};
}

void freeUnusedMemory() {
#ifdef __GLIBC__
  malloc_trim(4 * 1024 * 1024);
#endif
}

void traceMe() {
  // If the environment variable is defined, wait for a debugger.
  // In gdb, you need to invoke `signal SIGCONT` if you want ccls to continue
  // after detaching.
  const char *traceme = getenv("CCLS_TRACEME");
  if (traceme)
    raise(traceme[0] == 's' ? SIGSTOP : SIGTSTP);
}

void spawnThread(void *(*fn)(void *), void *arg) {
  pthread_t thd;
  pthread_attr_t attr;
  struct rlimit rlim;
  size_t stack_size = 4 * 1024 * 1024;
  if (getrlimit(RLIMIT_STACK, &rlim) == 0 && rlim.rlim_cur != RLIM_INFINITY)
    stack_size = rlim.rlim_cur;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setstacksize(&attr, stack_size);
  pipeline::threadEnter();
  pthread_create(&thd, &attr, fn, arg);
  pthread_attr_destroy(&attr);
}
} // namespace ccls

#endif
