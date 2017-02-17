#include "SourceLocation.h"
#include "Utility.h"

namespace clang {

SourceLocation::SourceLocation(CXTranslationUnit &tu, const std::string &filepath, unsigned offset) {
  CXFile file = clang_getFile(tu, filepath.c_str());
  cx_location = clang_getLocationForOffset(tu, file, offset);
}

SourceLocation::SourceLocation(CXTranslationUnit &tu, const std::string &filepath, unsigned line, unsigned column) {
  CXFile file = clang_getFile(tu, filepath.c_str());
  cx_location = clang_getLocation(tu, file, line, column);
}

std::string SourceLocation::get_path() {
  std::string path;
  get_data(&path, nullptr, nullptr, nullptr);
  return path;
}
Offset SourceLocation::get_offset() {
  unsigned line, index;
  get_data(nullptr, &line, &index, nullptr);
  return{ line, index };
}

void SourceLocation::get_data(std::string* path, unsigned *line, unsigned *column, unsigned *offset) {
  if (path == nullptr)
    clang_getExpansionLocation(cx_location, nullptr, line, column, offset);
  else {
    CXFile file;
    clang_getExpansionLocation(cx_location, &file, line, column, offset);
    if (file != nullptr) {
      *path = ToString(clang_getFileName(file));
    }
  }
}

}