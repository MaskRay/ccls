#pragma once

#include "maybe.h"
#include "serializer.h"
#include "utils.h"

#include <stdint.h>
#include <string>

struct Position {
  int16_t line;
  int16_t column;

  Position();
  Position(int16_t line, int16_t column);
  explicit Position(const char* encoded);

  bool HasValueForMaybe_() const { return line >= 0; }
  std::string ToString();
  std::string ToPrettyString(const std::string& filename);

  // Compare two Positions and check if they are equal. Ignores the value of
  // |interesting|.
  bool operator==(const Position& that) const;
  bool operator!=(const Position& that) const;
  bool operator<(const Position& that) const;
};
static_assert(
    sizeof(Position) == 4,
    "Investigate, Position should be 32-bits for indexer size reasons");
MAKE_HASHABLE(Position, t.line, t.column);

struct Range {
  Position start;
  Position end;

  Range();
  explicit Range(Position position);
  Range(Position start, Position end);
  explicit Range(const char* encoded);

  bool HasValueForMaybe_() const { return start.HasValueForMaybe_(); }
  bool Contains(int line, int column) const;
  Range RemovePrefix(Position position) const;

  std::string ToString();

  bool operator==(const Range& that) const;
  bool operator!=(const Range& that) const;
  bool operator<(const Range& that) const;
};
MAKE_HASHABLE(Range, t.start, t.end);

// Reflection
void Reflect(Reader& visitor, Position& value);
void Reflect(Writer& visitor, Position& value);
void Reflect(Reader& visitor, Range& value);
void Reflect(Writer& visitor, Range& value);
