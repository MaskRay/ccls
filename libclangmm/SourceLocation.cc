#include <cassert>

#include "SourceLocation.h"
#include "Utility.h"

namespace clang {

SourceLocation::SourceLocation(CXTranslationUnit &tu, const std::string &filepath, unsigned offset) {
  CXFile file = clang_getFile(tu, filepath.c_str());
  assert(false);
  //cx_location = clang_getLocationForOffset(tu, file, offset);
}

SourceLocation::SourceLocation(CXTranslationUnit &tu, const std::string &filepath, unsigned line, unsigned column) {
  CXFile file = clang_getFile(tu, filepath.c_str());
  assert(false);
  //cx_location = clang_getLocation(tu, file, line, column);
}

SourceLocation::SourceLocation(const CXSourceLocation& cx_location) {
  //clang_getExpansionLocation

  CXFile file;
  clang_getSpellingLocation(cx_location, &file, &line, &column, &offset);
  if (file != nullptr)
    path = clang::ToString(clang_getFileName(file));
}

std::string SourceLocation::ToString() const {
  return path + ":" + std::to_string(line) + ":" + std::to_string(column);
}

}