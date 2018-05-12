#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

void PlatformInit();

std::string GetExecutablePath();
std::string NormalizePath(const std::string& path);

void SetThreadName(const char* name);

// Free any unused memory and return it to the system.
void FreeUnusedMemory();

// Stop self and wait for SIGCONT.
void TraceMe();

std::string GetExternalCommandOutput(const std::vector<std::string>& command,
                                     std::string_view input);
