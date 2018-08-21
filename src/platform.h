// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <string_view>
#include <vector>

std::string NormalizePath(const std::string &path);

// Free any unused memory and return it to the system.
void FreeUnusedMemory();

// Stop self and wait for SIGCONT.
void TraceMe();

std::string GetExternalCommandOutput(const std::vector<std::string> &command,
                                     std::string_view input);
