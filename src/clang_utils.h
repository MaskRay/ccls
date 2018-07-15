#pragma once

#include <clang/Basic/FileManager.h>

#include <string>

// Returns the absolute path to |file|.
std::string FileName(const clang::FileEntry& file);

const char* ClangBuiltinTypeName(int);
