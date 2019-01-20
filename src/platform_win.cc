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

#if defined(_WIN32)
#include "platform.hh"

#include "utils.hh"
#include "log.hh"

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
std::string NormalizePath(const std::string &path) {
  DWORD retval = 0;
  TCHAR buffer[MAX_PATH] = TEXT("");
  TCHAR **lpp_part = {NULL};

  std::string result;
  retval = GetFullPathName(path.c_str(), MAX_PATH, buffer, lpp_part);
  // fail, return original
  if (retval == 0)
    result = path;
  else
    result = buffer;

  std::replace(result.begin(), result.end(), '\\', '/');
  // Normalize drive letter.
  if (result.size() > 1 && result[0] >= 'a' && result[0] <= 'z' &&
      result[1] == ':')
    result[0] = toupper(result[0]);
  return result;
}

void FreeUnusedMemory() {}

// TODO Wait for debugger to attach
void TraceMe() {}

std::string GetExternalCommandOutput(const std::vector<std::string> &command,
                                     std::string_view input) {
  return "";
}

void SpawnThread(void *(*fn)(void *), void *arg) {
  std::thread(fn, arg).detach();
}

void RemoveDirectoryRecursive(const std::string &path) {
    std::string pathString = path;  // create modifiable copy
    // parameter 'pFrom' must be double NULL-terminated:
    pathString.append(2, 0);

    // We use SHFileOperation, because its FO_DELETE operation is recursive.
    SHFILEOPSTRUCT op;
    memset(&op, 0, sizeof(op));
    op.wFunc = FO_DELETE;
    op.pFrom = pathString.c_str();
    op.fFlags = FOF_NO_UI;
    SHFileOperation(&op);
}

std::optional<std::string> TryMakeTempDirectory() {
    // get "temp" dir
    char tmpdir_buf[MAX_PATH];
    DWORD len = GetTempPath(MAX_PATH, tmpdir_buf);

    // Unfortunately, there is no mkdtemp() on windows. We append a (random)
    // GUID to use as a unique directory name.
    GUID guid;
    CoCreateGuid(&guid);
    // simplest way to append the guid to the existing c string:
    len += snprintf(tmpdir_buf + len, MAX_PATH - len,
        "ccls-%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
             guid.Data1, guid.Data2, guid.Data3,
             guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
             guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

    std::string dirPath(tmpdir_buf, len);

    // finally, create the dir
    const bool createSuccessful = (_mkdir(dirPath.c_str()) != -1) ||
                                  (errno == EEXIST);

    if(createSuccessful) return dirPath;
    return std::nullopt;
}

}

#endif
