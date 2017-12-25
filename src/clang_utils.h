#pragma once

#include "language_server_api.h"

#include <clang-c/Index.h>
#include <clang/Format/Format.h>
#include <optional.h>

#include <vector>

using namespace std::experimental;

optional<lsDiagnostic> BuildAndDisposeDiagnostic(CXDiagnostic diagnostic,
                                                 const std::string& path);

// Returns the absolute path to |file|.
std::string FileName(CXFile file);

std::string ToString(CXString cx_string);

std::string ToString(CXCursorKind cursor_kind);

// Converts Clang formatting replacement operations into LSP text edits.
std::vector<lsTextEdit> ConvertClangReplacementsIntoTextEdits(
     llvm::StringRef document,
     const std::vector<clang::tooling::Replacement>& clang_replacements);
