#include "SourceRange.h"

clang::SourceRange::SourceRange(clang::SourceLocation &start, clang::SourceLocation &end) {
  cx_range = clang_getRange(start.cx_location, end.cx_location);
}

std::pair<clang::Offset, clang::Offset> clang::SourceRange::get_offsets() {
  SourceLocation start(clang_getRangeStart(cx_range)), end(clang_getRangeEnd(cx_range));
  return {start.get_offset(), end.get_offset()};
}