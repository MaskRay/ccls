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
#include "platform.h"

#include "utils.h"

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

#include <string>

namespace {

// Returns the canonicalized absolute pathname, without expanding symbolic
// links. This is a variant of realpath(2), C++ rewrite of
// https://github.com/freebsd/freebsd/blob/master/lib/libc/stdlib/realpath.c
std::optional<std::string> RealPathNotExpandSymlink(std::string path) {
  if (path.empty()) {
    errno = EINVAL;
    return std::nullopt;
  }
  if (path[0] == '\0') {
    errno = ENOENT;
    return std::nullopt;
  }

  // Do not use PATH_MAX because it is tricky on Linux.
  // See https://eklitzke.org/path-max-is-tricky
  char tmp[1024];
  std::string resolved;
  size_t i = 0;
  struct stat sb;
  if (path[0] == '/') {
    resolved = "/";
    i = 1;
  } else {
    if (!getcwd(tmp, sizeof tmp))
      return std::nullopt;
    resolved = tmp;
  }

  while (i < path.size()) {
    auto j = path.find('/', i);
    if (j == std::string::npos)
      j = path.size();
    auto next_token = path.substr(i, j - i);
    i = j + 1;
    if (resolved.back() != '/')
      resolved += '/';
    if (next_token.empty() || next_token == ".") {
      // Handle consequential slashes and "."
      continue;
    } else if (next_token == "..") {
      // Strip the last path component except when it is single "/"
      if (resolved.size() > 1)
        resolved.resize(resolved.rfind('/', resolved.size() - 2) + 1);
      continue;
    }
    // Append the next path component.
    // Here we differ from realpath(3), we use stat(2) instead of
    // lstat(2) because we do not want to resolve symlinks.
    resolved += next_token;
    if (stat(resolved.c_str(), &sb) != 0)
      return std::nullopt;
    if (!S_ISDIR(sb.st_mode) && j < path.size()) {
      errno = ENOTDIR;
      return std::nullopt;
    }
  }

  // Remove trailing slash except when a single "/".
  if (resolved.size() > 1 && resolved.back() == '/')
    resolved.pop_back();
  return resolved;
}

} // namespace

std::string NormalizePath(const std::string &path) {
  std::optional<std::string> resolved = RealPathNotExpandSymlink(path);
  return resolved ? *resolved : path;
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
  if (getrlimit(RLIMIT_STACK, &rlim) == 0 &&
      rlim.rlim_cur != RLIM_INFINITY)
    stack_size = rlim.rlim_cur;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setstacksize(&attr, stack_size);
  pthread_create(&thd, &attr, fn, arg);
  pthread_attr_destroy(&attr);
}

#endif
