#pragma once

#include "clang_cursor.h"
#include "clang_index.h"

#include <clang-c/Index.h>

#include <memory>
#include <string>
#include <vector>

// RAII wrapper around CXTranslationUnit which also makes it much more
// challenging to use a CXTranslationUnit instance that is not correctly
// initialized.
class ClangTranslationUnit {
 public:
  static std::unique_ptr<ClangTranslationUnit> Create(
      ClangIndex* index,
      const std::string& filepath,
      const std::vector<std::string>& arguments,
      std::vector<CXUnsavedFile> unsaved_files,
      unsigned flags);

  static std::unique_ptr<ClangTranslationUnit> Reparse(
      std::unique_ptr<ClangTranslationUnit> tu,
      std::vector<CXUnsavedFile>& unsaved);

  explicit ClangTranslationUnit(CXTranslationUnit tu);
  ~ClangTranslationUnit();

  CXTranslationUnit cx_tu;
};