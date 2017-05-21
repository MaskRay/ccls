#pragma once

#include "language_server_api.h"

#include <clang-c/Index.h>

#include <vector>
#include <optional.h>

using namespace std::experimental;

optional<lsDiagnostic> BuildDiagnostic(CXDiagnostic diagnostic);

// Returns the absolute path to |file|.
std::string FileName(CXFile file);
