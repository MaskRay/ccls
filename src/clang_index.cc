#include "clang_index.h"

ClangIndex::ClangIndex() : ClangIndex(1, 0) {}

ClangIndex::ClangIndex(int exclude_declarations_from_pch,
                       int display_diagnostics) {
  cx_index =
      clang_createIndex(exclude_declarations_from_pch, display_diagnostics);
}

ClangIndex::~ClangIndex() {
  clang_disposeIndex(cx_index);
}