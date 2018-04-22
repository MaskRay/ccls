#if defined(__unix__) || defined(__APPLE__)
#include "platform.h"

#include "utils.h"

#include "loguru.hpp"

#include <pthread.h>
#if defined(__FreeBSD__)
#include <pthread_np.h>
#include <sys/thr.h>
#elif defined(__OpenBSD__)
#include <pthread_np.h>
#endif

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
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>

#include <semaphore.h>
#include <sys/mman.h>

#if defined(__FreeBSD__)
#include <sys/param.h>   // MAXPATHLEN
#include <sys/sysctl.h>  // sysctl
#elif defined(__linux__)
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

}  // namespace

void PlatformInit() {}

#ifdef __APPLE__
extern "C" int _NSGetExecutablePath(char* buf, uint32_t* bufsize);
#endif

// See
// https://stackoverflow.com/questions/143174/how-do-i-get-the-directory-that-a-program-is-running-from
std::string GetExecutablePath() {
#ifdef __APPLE__
  uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  char* buffer = new char[size];
  _NSGetExecutablePath(buffer, &size);
  char* resolved = realpath(buffer, nullptr);
  std::string result(resolved);
  delete[] buffer;
  free(resolved);
  return result;
#elif defined(__FreeBSD__)
  static const int name[] = {
      CTL_KERN,
      KERN_PROC,
      KERN_PROC_PATHNAME,
      -1,
  };
  char path[MAXPATHLEN];
  size_t len = sizeof(path);
  path[0] = '\0';
  (void)sysctl(name, 4, path, &len, NULL, 0);
  return std::string(path);
#else
  char buffer[PATH_MAX] = {0};
  if (-1 == readlink("/proc/self/exe", buffer, PATH_MAX))
    return "";
  return buffer;
#endif
}

std::string NormalizePath(const std::string& path) {
  std::optional<std::string> resolved = RealPathNotExpandSymlink(path);
  return resolved ? *resolved : path;
}

void SetThreadName(const std::string& thread_name) {
  loguru::set_thread_name(thread_name.c_str());
#if defined(__APPLE__)
  pthread_setname_np(thread_name.c_str());
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
  pthread_set_name_np(pthread_self(), thread_name.c_str());
#elif defined(__linux__)
  pthread_setname_np(pthread_self(), thread_name.c_str());
#endif
}

void FreeUnusedMemory() {
#if defined(__GLIBC__)
  malloc_trim(0);
#endif
}

void TraceMe() {
  // If the environment variable is defined, wait for a debugger.
  // In gdb, you need to invoke `signal SIGCONT` if you want ccls to continue
  // after detaching.
  if (getenv("CCLS_TRACEME"))
    raise(SIGSTOP);
}

std::string GetExternalCommandOutput(const std::vector<std::string>& command,
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
    auto argv = new char*[command.size() + 1];
    for (size_t i = 0; i < command.size(); i++)
      argv[i] = const_cast<char*>(command[i].c_str());
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

#endif
