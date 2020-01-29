// Copyright 2017-2020 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <llvm/ADT/StringRef.h>

#include <string>

namespace ccls {
std::string normalizePath(llvm::StringRef path);

// Free any unused memory and return it to the system.
void freeUnusedMemory();

// Stop self and wait for SIGCONT.
void traceMe();

void spawnThread(void *(*fn)(void *), void *arg);
} // namespace ccls
