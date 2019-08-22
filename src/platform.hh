// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace ccls {
std::string normalizePath(const std::string &path);

// Free any unused memory and return it to the system.
void freeUnusedMemory();

// Stop self and wait for SIGCONT.
void traceMe();

void spawnThread(void *(*fn)(void *), void *arg);
} // namespace ccls
