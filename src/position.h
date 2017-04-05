#pragma once

#include <cassert>
#include <cstdint>
#include <string>

struct Position {
  bool interesting = false;
  int32_t line = -1;
  int32_t column = -1;

  Position();
  Position(bool interesting, int32_t line, int32_t column);
  explicit Position(const char* encoded);

  std::string ToString();
  std::string ToPrettyString(const std::string& filename);

  Position WithInteresting(bool interesting);

  // Compare two Positions and check if they are equal. Ignores the value of
  // |interesting|.
  bool operator==(const Position& that) const;
  bool operator!=(const Position& that) const;
  bool operator<(const Position& that) const;
};

struct Range {
  Position start;
  Position end;

  Range();
  Range(Position start, Position end);
  explicit Range(const char* encoded);

  std::string ToString();
  Range WithInteresting(bool interesting);

  bool operator==(const Range& that) const;
  bool operator!=(const Range& that) const;
  bool operator<(const Range& that) const;
};