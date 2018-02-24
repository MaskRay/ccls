#pragma once

#include "position.h"

#include "optional.h"

#include <string>
#include <unordered_map>
#include <vector>

struct FileContents {
  FileContents();
  FileContents(const std::string& path, const std::string& content);

  optional<int> ToOffset(Position p) const;
  optional<std::string> ContentsInRange(Range range) const;

  std::string path;
  std::string content;
  // {0, 1 + position of first newline, 1 + position of second newline, ...}
  std::vector<int> line_offsets_;
};

using FileContentsMap = std::unordered_map<std::string, FileContents>;
