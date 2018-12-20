/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#if defined(__unix__) || defined(__APPLE__)
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
void ThreadEnter();
}

std::string NormalizePath(const std::string &path) {
  llvm::SmallString<256> P(path);
  llvm::sys::path::remove_dots(P, true);
  return {P.data(), P.size()};
}

void FreeUnusedMemory() {
#ifdef __GLIBC__
  malloc_trim(4 * 1024 * 1024);
#endif
}

void TraceMe() {
  // If the environment variable is defined, wait for a debugger.
  // In gdb, you need to invoke `signal SIGCONT` if you want ccls to continue
  // after detaching.
  const char *traceme = getenv("CCLS_TRACEME");
  if (traceme)
    raise(traceme[0] == 's' ? SIGSTOP : SIGTSTP);
}

std::string GetExternalCommandOutput(const std::vector<std::string> &command,
                                     std::string_view input) {
  int pin[2], pout[2];
  if (pipe(pin) < 0) {
    perror("pipe(stdin)");
    return "";
  }
  if (pipe(pout) < 0) {
    perror("pipe(stdout)");
    close(pin[0]);
    close(pin[1]);
    return "";
  }
  pid_t child = fork();
  if (child == 0) {
    dup2(pout[0], 0);
    dup2(pin[1], 1);
    close(pin[0]);
    close(pin[1]);
    close(pout[0]);
    close(pout[1]);
    auto argv = new char *[command.size() + 1];
    for (size_t i = 0; i < command.size(); i++)
      argv[i] = const_cast<char *>(command[i].c_str());
    argv[command.size()] = nullptr;
    execvp(argv[0], argv);
    _Exit(127);
  }
  close(pin[1]);
  close(pout[0]);
  // O_NONBLOCK is disabled, write(2) blocks until all bytes are written.
  (void)write(pout[1], input.data(), input.size());
  close(pout[1]);
  std::string ret;
  char buf[4096];
  ssize_t n;
  while ((n = read(pin[0], buf, sizeof buf)) > 0)
    ret.append(buf, n);
  close(pin[0]);
  waitpid(child, NULL, 0);
  return ret;
}

void SpawnThread(void *(*fn)(void *), void *arg) {
  pthread_t thd;
  pthread_attr_t attr;
  struct rlimit rlim;
  size_t stack_size = 4 * 1024 * 1024;
  if (getrlimit(RLIMIT_STACK, &rlim) == 0 && rlim.rlim_cur != RLIM_INFINITY)
    stack_size = rlim.rlim_cur;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setstacksize(&attr, stack_size);
  pipeline::ThreadEnter();
  pthread_create(&thd, &attr, fn, arg);
  pthread_attr_destroy(&attr);
}
} // namespace ccls

#endif
