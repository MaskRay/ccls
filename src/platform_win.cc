#if defined(_WIN32)
#include "platform.h"

#include "utils.h"

#include <loguru.hpp>

#include <Windows.h>
#include <direct.h>
#include <fcntl.h>
#include <io.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cassert>
#include <string>

void PlatformInit() {
  // We need to write to stdout in binary mode because in Windows, writing
  // \n will implicitly write \r\n. Language server API will ignore a
  // \r\r\n split request.
  _setmode(_fileno(stdout), O_BINARY);
  _setmode(_fileno(stdin), O_BINARY);
}

// See
// https://stackoverflow.com/questions/143174/how-do-i-get-the-directory-that-a-program-is-running-from
std::string GetExecutablePath() {
  char result[MAX_PATH] = {0};
  GetModuleFileName(NULL, result, MAX_PATH);
  return NormalizePath(result);
}

std::string NormalizePath(const std::string& path) {
  DWORD retval = 0;
  TCHAR buffer[MAX_PATH] = TEXT("");
  TCHAR** lpp_part = {NULL};

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

std::string GetExternalCommandOutput(const std::vector<std::string>& command,
                                     std::string_view input) {
  return "";
}

#endif
