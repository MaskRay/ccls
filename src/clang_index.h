#pragma once

#include <clang-c/Index.h>

// Simple RAII wrapper about CXIndex.
class ClangIndex {
 public:
  ClangIndex();
  ClangIndex(int exclude_declarations_from_pch, int display_diagnostics);
  ~ClangIndex();
  CXIndex cx_index;
};
