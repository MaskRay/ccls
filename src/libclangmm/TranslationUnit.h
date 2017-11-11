#pragma once

#include "../clang_cursor.h"
#include "Index.h"

#include <clang-c/Index.h>

#include <memory>
#include <string>
#include <vector>

namespace clang {

class TranslationUnit {
 public:
  static std::unique_ptr<TranslationUnit> Create(
      Index* index,
      const std::string& filepath,
      const std::vector<std::string>& arguments,
      std::vector<CXUnsavedFile> unsaved_files,
      unsigned flags);

  static std::unique_ptr<TranslationUnit> Reparse(
      std::unique_ptr<TranslationUnit> tu,
      std::vector<CXUnsavedFile>& unsaved);

  explicit TranslationUnit(CXTranslationUnit tu);
  ~TranslationUnit();

  CXTranslationUnit cx_tu;
};

}  // namespace clang
