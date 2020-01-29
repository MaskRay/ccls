// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#if defined(_WIN32)
#include "platform.hh"

#include "utils.hh"

#include <Windows.h>
#include <direct.h>
#include <fcntl.h>
#include <io.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <string>
#include <thread>

namespace ccls {
std::string normalizePath(llvm::StringRef path) {
  TCHAR buffer[MAX_PATH] = TEXT("");
  TCHAR **lpp_part = {NULL};

  std::string result(path);
  if (GetFullPathName(result.c_str(), MAX_PATH, buffer, lpp_part) != 0)
    result = buffer;

  std::replace(result.begin(), result.end(), '\\', '/');
  // Normalize drive letter.
  if (result.size() > 1 && result[0] >= 'a' && result[0] <= 'z' &&
      result[1] == ':')
    result[0] = toupper(result[0]);
  return result;
}

void freeUnusedMemory() {}

// TODO Wait for debugger to attach
void traceMe() {}

void spawnThread(void *(*fn)(void *), void *arg) {
  std::thread(fn, arg).detach();
}
} // namespace ccls

#endif
