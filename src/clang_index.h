#pragma once

#include <clang-c/Index.h>

// Simple RAII wrapper about CXIndex.
// Note: building a ClangIndex instance acquires a global lock, since libclang
// API does not appear to be thread-safe here.
class ClangIndex {
 public:
  ClangIndex();
  ClangIndex(int exclude_declarations_from_pch, int display_diagnostics);
  ~ClangIndex();
  CXIndex cx_index;
};
