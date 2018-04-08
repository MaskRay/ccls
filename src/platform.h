#pragma once

#include <optional>
#include <string_view>

#include <memory>
#include <string>
#include <vector>

void PlatformInit();

std::string GetExecutablePath();
std::string NormalizePath(const std::string& path);

void SetThreadName(const std::string& thread_name);

std::optional<int64_t> GetLastModificationTime(const std::string& absolute_path);

// Free any unused memory and return it to the system.
void FreeUnusedMemory();

// Stop self and wait for SIGCONT.
void TraceMe();

std::string GetExternalCommandOutput(const std::vector<std::string>& command,
                                     std::string_view input);
