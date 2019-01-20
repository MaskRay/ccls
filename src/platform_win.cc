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

class Utf8To16 {
public:
  Utf8To16(const char *utf8String);

  operator const wchar_t      *  () const {return m_str.c_str();}
  operator const std::wstring &  () const {return m_str;}

  const std::wstring &asWstring() const {return m_str;}
  const wchar_t      *asWchar_t() const {return m_str.c_str();}

private:
  std::wstring m_str;
};

Utf8To16::Utf8To16(const char *utf8Str) {
  const std::size_t inSize = ::strlen(utf8Str);

  // find out how many utf16 characters we need
  const int requiredU16Chars =
      MultiByteToWideChar(CP_UTF8, 0, utf8Str, inSize, nullptr, 0);
  m_str.resize(requiredU16Chars);

  // finally, do the conversion
  MultiByteToWideChar(CP_UTF8, 0, utf8Str, inSize, &m_str[0], m_str.size());
}

void closeHandleIfValid(HANDLE &f_handle) {
    if(f_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(f_handle);
        f_handle = INVALID_HANDLE_VALUE;
    }
}

std::string readAllFromPipe(HANDLE pipe) {
  std::string ret;
  const int BUFFER_SIZE = 1024;
  CHAR buf[BUFFER_SIZE];
  DWORD bytesRead = 0;
  bool moreToRead = true;

  while (moreToRead) {
    if (ReadFile(pipe, buf, BUFFER_SIZE, &bytesRead, NULL)) {
      ret.append(buf, bytesRead);
    } else {
      const DWORD err = GetLastError();

      if(err == ERROR_BROKEN_PIPE) {
        // child process terminated (this is not an error)
      } else {
        LOG_S(ERROR) << "Error while reading from child process: " << err;
      }
      moreToRead = false;
    }
  }
  return ret;
}

std::string GetExternalCommandOutput(const std::vector<std::string> &command,
                                     std::string_view input) {
  SECURITY_ATTRIBUTES saAttr;
  saAttr.nLength = sizeof(saAttr);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = nullptr;

  enum { READ_END, WRITE_END, NUM_HANDLES };

  HANDLE stdIn[NUM_HANDLES];
  HANDLE stdOut[NUM_HANDLES];
  HANDLE stdErr[NUM_HANDLES];

  // create anonymous pipes for the child process
  if (!CreatePipe(&stdIn[READ_END], &stdIn[WRITE_END], &saAttr, 0) ||
      !CreatePipe(&stdOut[READ_END], &stdOut[WRITE_END], &saAttr, 0) ||
      !CreatePipe(&stdErr[READ_END], &stdErr[WRITE_END], &saAttr, 0)) {
    LOG_S(ERROR) << "Error creating pipes for child process";
    return "";
  }

  // the child is not supposed to gain access to the pipes' parent end
  if(!SetHandleInformation(stdIn[WRITE_END], HANDLE_FLAG_INHERIT, 0) ||
     !SetHandleInformation(stdOut[READ_END], HANDLE_FLAG_INHERIT, 0) ||
     !SetHandleInformation(stdErr[READ_END], HANDLE_FLAG_INHERIT, 0))
  {
    LOG_S(ERROR) << "Error in SetHandleInformation: " << GetLastError();
    return "";
  }

  // set up STARTUPINFO structure. It tells CreateProcess to use the pipes
  // we just created as stdin and stdout for the new process.
  STARTUPINFOW siStartInfo;
  memset(&siStartInfo, 0, sizeof(siStartInfo));
  siStartInfo.cb = sizeof(siStartInfo);
  siStartInfo.hStdInput  = stdIn[READ_END];
  siStartInfo.hStdOutput = stdOut[WRITE_END];
  siStartInfo.hStdError = stdErr[WRITE_END];
  siStartInfo.dwFlags   |= STARTF_USESTDHANDLES;

  PROCESS_INFORMATION process;
  memset(&process, 0, sizeof(process));

  // fold the command line parts into a single string, quoting the arguments
  std::string cmdString;
  for (int i = 0; i < command.size(); i++) {
    cmdString += ((i == 0) ? command[i] : (" \"" + command[i] + "\"")) ;
  }

  std::wstring cmdString_u16 = Utf8To16(cmdString.c_str());
  if(!CreateProcessW(NULL,  // app name: we pass it through lpCommandLine
                     &cmdString_u16[0],
                     NULL,  // security attrs
                     NULL,  // thread security attrs
                     TRUE,  // handles are inherited
                     CREATE_UNICODE_ENVIRONMENT, // creation flags
                     NULL,  // environment
                     NULL,  // cwd
                     &siStartInfo,  // in: stdin, stdout, stderr pipes
                     &process    // out: info about the new process
       )) {
    LOG_S(ERROR) << "Error creating process " << GetLastError();
    return "";
  }

  // we need to close our handles to the write end of these pipe. Otherwise,
  // ReadFile() will not return when the child process terminates.
  closeHandleIfValid(stdOut[WRITE_END]);
  closeHandleIfValid(stdErr[WRITE_END]);
  closeHandleIfValid(stdIn[READ_END]);

  // write input data to process' stdin
  DWORD numBytesOut = 0;
  WriteFile(stdIn[WRITE_END], input.data(), input.size(), &numBytesOut, NULL);
  closeHandleIfValid(stdIn[WRITE_END]);  // close the handle to signal data end

  // wait for the process to finish
  if(process.hProcess != INVALID_HANDLE_VALUE) {
    DWORD res = WaitForSingleObject(process.hProcess, INFINITE);
    if(res != WAIT_OBJECT_0) {
      LOG_S(ERROR) << "Error waiting for process to finish: " << res;
      return "";
    }
  }

  // Process return code is not used at the moment, but if it's required later,
  // here's the necessary code for it:
  // DWORD retCode;
  // GetExitCodeProcess(process.hProcess, &retCode);

  // read all stdout/stderr that the process has produced:
  std::string output = readAllFromPipe(stdOut[READ_END]);
  std::string err = readAllFromPipe(stdErr[READ_END]);

  if (err.size() > 0) {
    LOG_S(ERROR) << "Stderr from child process:\n" << err;
  }

  // close all the handles
  for(int i=0; i<NUM_HANDLES; i++) {
    closeHandleIfValid(stdIn[i]);
    closeHandleIfValid(stdOut[i]);
    closeHandleIfValid(stdErr[i]);
  }
  closeHandleIfValid(process.hThread);
  closeHandleIfValid(process.hProcess);

  return output;
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
  const bool createSuccessful =
    (_mkdir(dirPath.c_str()) != -1) || (errno == EEXIST);

  if(createSuccessful) return dirPath;
  return std::nullopt;
}

}

#endif
