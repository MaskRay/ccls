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

SourceLocation::SourceLocation() {}

SourceLocation::SourceLocation(const CXSourceLocation& cx_location) {
  //clang_getExpansionLocation

  CXFile file;
  clang_getSpellingLocation(cx_location, &file, &line, &column, &offset);
  if (file != nullptr)
    path = clang::ToString(clang_getFileName(file));
}

SourceLocation::SourceLocation(const CXIdxLoc& cx_location) 
  : SourceLocation(clang_indexLoc_getCXSourceLocation(cx_location)) {
}

bool SourceLocation::operator==(const SourceLocation& o) {
  return path == o.path && line == o.line && column == o.column;
}

bool SourceLocation::operator!=(const SourceLocation& o) {
  return !(*this == o);
}

std::string SourceLocation::ToString() const {
  return path + ":" + std::to_string(line) + ":" + std::to_string(column);
}

}