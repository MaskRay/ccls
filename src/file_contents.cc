#include "file_contents.h"

FileContents::FileContents() : line_offsets_{0} {}

FileContents::FileContents(const std::string& path, const std::string& content)
    : path(path), content(content) {
  line_offsets_.push_back(0);
  for (size_t i = 0; i < content.size(); i++) {
    if (content[i] == '\n')
      line_offsets_.push_back(i + 1);
  }
}

optional<int> FileContents::ToOffset(Position p) const {
  if (0 < p.line && size_t(p.line) <= line_offsets_.size()) {
    int ret = line_offsets_[p.line - 1] + p.column - 1;
    if (size_t(ret) <= content.size())
      return ret;
  }
  return nullopt;
}

optional<std::string> FileContents::ContentsInRange(Range range) const {
  optional<int> start_offset = ToOffset(range.start),
                end_offset = ToOffset(range.end);
  if (start_offset && end_offset && *start_offset < *end_offset)
    return content.substr(*start_offset, *end_offset - *start_offset);
  return nullopt;
}