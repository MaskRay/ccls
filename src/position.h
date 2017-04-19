#pragma once

#include <cassert>
#include <cstdint>
#include <string>

struct Position {
  int16_t line = -1;
  int16_t column = -1;

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
static_assert(sizeof(Position) == 4, "Investigate, Position should be 32-bits for indexer size reasons");

struct Range {
  Position start;
  Position end;

  Range();
  Range(Position start, Position end);
  explicit Range(const char* encoded);

  bool Contains(int line, int column) const;

  std::string ToString();

  bool operator==(const Range& that) const;
  bool operator!=(const Range& that) const;
  bool operator<(const Range& that) const;
};