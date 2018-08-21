// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <clang/Basic/FileManager.h>

#include <string>

// Returns the absolute path to |file|.
std::string FileName(const clang::FileEntry &file);

const char *ClangBuiltinTypeName(int);
