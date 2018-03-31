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
#include <iostream>
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

// See http://stackoverflow.com/a/19535628
std::string GetWorkingDirectory() {
  char result[MAX_PATH];
  std::string binary_path(result, GetModuleFileName(NULL, result, MAX_PATH));
  return binary_path.substr(0, binary_path.find_last_of("\\/") + 1);
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

bool TryMakeDirectory(const std::string& absolute_path) {
  if (_mkdir(absolute_path.c_str()) == -1) {
    // Success if the directory exists.
    return errno == EEXIST;
  }
  return true;
}

// See https://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO {
  DWORD dwType;      // Must be 0x1000.
  LPCSTR szName;     // Pointer to name (in user addr space).
  DWORD dwThreadID;  // Thread ID (-1=caller thread).
  DWORD dwFlags;     // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
void SetCurrentThreadName(const std::string& thread_name) {
  loguru::set_thread_name(thread_name.c_str());

  THREADNAME_INFO info;
  info.dwType = 0x1000;
  info.szName = thread_name.c_str();
  info.dwThreadID = (DWORD)-1;
  info.dwFlags = 0;

  __try {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR),
                   (ULONG_PTR*)&info);
#ifdef _MSC_VER
  } __except (EXCEPTION_EXECUTE_HANDLER) {
#else
  } catch (...) {
#endif
  }
}

std::optional<int64_t> GetLastModificationTime(const std::string& absolute_path) {
  struct _stat buf;
  if (_stat(absolute_path.c_str(), &buf) != 0) {
    switch (errno) {
      case ENOENT:
        // std::cerr << "GetLastModificationTime: unable to find file " <<
        // absolute_path << std::endl;
        return std::nullopt;
      case EINVAL:
        // std::cerr << "GetLastModificationTime: invalid param to _stat for
        // file file " << absolute_path << std::endl;
        return std::nullopt;
      default:
        // std::cerr << "GetLastModificationTime: unhandled for " <<
        // absolute_path << std::endl;  exit(1);
        return std::nullopt;
    }
  }

  return buf.st_mtime;
}

void MoveFileTo(const std::string& destination, const std::string& source) {
  MoveFile(source.c_str(), destination.c_str());
}

void CopyFileTo(const std::string& destination, const std::string& source) {
  CopyFile(source.c_str(), destination.c_str(), false /*failIfExists*/);
}

bool IsSymLink(const std::string& path) {
  return false;
}

void FreeUnusedMemory() {}

bool RunObjectiveCIndexTests() {
  return false;
}

// TODO Wait for debugger to attach
void TraceMe() {}

std::string GetExternalCommandOutput(const std::vector<std::string>& command,
                                     std::string_view input) {
  return "";
}

#endif
