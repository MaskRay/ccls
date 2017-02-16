#ifndef SOURCELOCATION_H_
#define SOURCELOCATION_H_

#include <clang-c/Index.h>
#include <string>

namespace clang {

class Offset {
public:
  Offset() {}
  Offset(unsigned line, unsigned index) : line(line), index(index) {}
  bool operator==(const clang::Offset &o) { return (line == o.line && index == o.index); }
  bool operator!=(const clang::Offset &o) { return !(*this == o); }
  unsigned line;
  unsigned index; //byte index in line (not char number)
};

class SourceLocation {
  friend class TranslationUnit;
  SourceLocation(CXTranslationUnit &tu, const std::string &filepath, unsigned offset);
  SourceLocation(CXTranslationUnit &tu, const std::string &filepath, unsigned line, unsigned column);
public:
  SourceLocation(const CXSourceLocation& cx_location) : cx_location(cx_location) {}

public:
  std::string get_path();
  clang::Offset get_offset();

  CXSourceLocation cx_location;

private:
  void get_data(std::string *path, unsigned *line, unsigned *column, unsigned *offset);
};

}  // namespace clang

#endif  // SOURCELOCATION_H_
