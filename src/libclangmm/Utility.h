#pragma once

#include <clang-c/Index.h>
#include <string>

namespace clang {

std::string ToString(CXString cx_string);
std::string ToString(CXCursorKind cursor_kind);

}  // namespace clang