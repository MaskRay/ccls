// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#if defined(_WIN32)
#include "platform.h"

#include "utils.h"

#include <Windows.h>
#include <direct.h>
#include <fcntl.h>
#include <io.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cassert>
#include <string>

std::string NormalizePath(const std::string &path) {
  DWORD retval = 0;
  TCHAR buffer[MAX_PATH] = TEXT("");
  TCHAR **lpp_part = {NULL};

  retval = GetFullPathName(path.c_str(), MAX_PATH, buffer, lpp_part);
  // fail, return original
  if (retval == 0)
    return path;

  std::string result = buffer;
  std::replace(result.begin(), result.end(), '\\', '/');
  // std::transform(result.begin(), result.end(), result.begin(), ::tolower);
  return result;
}

void FreeUnusedMemory() {}

// TODO Wait for debugger to attach
void TraceMe() {}

std::string GetExternalCommandOutput(const std::vector<std::string> &command,
                                     std::string_view input) {
  return "";
}

#endif
