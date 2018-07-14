#pragma once

#include "lsp_diagnostic.h"

#include <clang-c/Index.h>
#include <clang/Basic/FileManager.h>

#include <optional>
#include <vector>

// Returns the absolute path to |file|.
std::string FileName(CXFile file);
std::string FileName(const clang::FileEntry& file);

std::string ToString(CXString cx_string);

std::string ToString(CXCursorKind cursor_kind);

const char* ClangBuiltinTypeName(int);
