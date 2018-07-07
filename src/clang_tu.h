#pragma once
#include "position.h"

#include <clang-c/Index.h>

#include <memory>
#include <string>
#include <vector>

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

// RAII wrapper around CXTranslationUnit which also makes it much more
// challenging to use a CXTranslationUnit instance that is not correctly
// initialized.
struct ClangTranslationUnit {
  static std::unique_ptr<ClangTranslationUnit> Create(
      ClangIndex* index,
      const std::string& filepath,
      const std::vector<std::string>& arguments,
      std::vector<CXUnsavedFile>& unsaved_files,
      unsigned flags);

  static std::unique_ptr<ClangTranslationUnit> Reparse(
      std::unique_ptr<ClangTranslationUnit> tu,
      std::vector<CXUnsavedFile>& unsaved);

  explicit ClangTranslationUnit(CXTranslationUnit tu);
  ~ClangTranslationUnit();

  CXTranslationUnit cx_tu;
};
