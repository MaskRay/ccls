#pragma once

#include <cassert>
#include <cstdint>
#include <string>

struct Position {
  int32_t line = -1;
  int32_t column = -1;

  Position();
  Position(int32_t line, int32_t column);
  explicit Position(const char* encoded);

  std::string ToString();
  std::string ToPrettyString(const std::string& filename);

  // Compare two Positions and check if they are equal. Ignores the value of
  // |interesting|.
  bool operator==(const Position& that) const;
  bool operator!=(const Position& that) const;
  bool operator<(const Position& that) const;
};

struct Range {
  bool interesting = false;
  Position start;
  Position end;

  Range();
  Range(bool interesting, Position start, Position end);
  explicit Range(const char* encoded);

  bool Contains(int line, int column) const;

  std::string ToString();
  Range WithInteresting(bool interesting);

  bool operator==(const Range& that) const;
  bool operator!=(const Range& that) const;
  bool operator<(const Range& that) const;
};