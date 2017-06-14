#pragma once


#include "Index.h"
#include "Cursor.h"

#include <clang-c/Index.h>

#include <string>
#include <vector>

namespace clang {

class TranslationUnit {
 public:
  TranslationUnit(Index* index,
                  const std::string& filepath,
                  const std::vector<std::string>& arguments,
                  std::vector<CXUnsavedFile> unsaved_files,
                  unsigned flags);
  ~TranslationUnit();

  bool did_fail = false;

  void ReparseTranslationUnit(std::vector<CXUnsavedFile>& unsaved);

  Cursor document_cursor() const;

  CXTranslationUnit cx_tu;
};

}  // namespace clang
