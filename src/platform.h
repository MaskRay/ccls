#pragma once

#include <optional.h>
#include <string_view.h>

#include <memory>
#include <string>
#include <vector>

struct PlatformMutex {
  virtual ~PlatformMutex();
};
struct PlatformScopedMutexLock {
  virtual ~PlatformScopedMutexLock();
};
struct PlatformSharedMemory {
  virtual ~PlatformSharedMemory();
  void* data;
  size_t capacity;
  std::string name;
};

void PlatformInit();

std::string GetExecutablePath();
std::string GetWorkingDirectory();
std::string NormalizePath(const std::string& path);
// Creates a directory at |path|. Creates directories recursively if needed.
void MakeDirectoryRecursive(std::string path);
// Tries to create the directory given by |absolute_path|. Returns true if
// successful or if the directory already exists. Returns false otherwise. This
// does not attempt to recursively create directories.
bool TryMakeDirectory(const std::string& absolute_path);

void SetCurrentThreadName(const std::string& thread_name);

optional<int64_t> GetLastModificationTime(const std::string& absolute_path);

void MoveFileTo(const std::string& destination, const std::string& source);
void CopyFileTo(const std::string& destination, const std::string& source);

bool IsSymLink(const std::string& path);

// Returns any clang arguments that are specific to the current platform.
std::vector<const char*> GetPlatformClangArguments();

// Free any unused memory and return it to the system.
void FreeUnusedMemory();

// If true objective-c index tests will be run.
bool RunObjectiveCIndexTests();

// Stop self and wait for SIGCONT.
void TraceMe();

std::string GetExternalCommandOutput(const std::vector<std::string>& command,
                                     std::string_view input);
