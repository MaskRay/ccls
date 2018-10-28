// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "maybe.h"
#include "utils.h"

#include <stdint.h>
#include <string>

namespace ccls {
struct Position {
  int16_t line = -1;
  int16_t column = -1;

  static Position FromString(const std::string &encoded);

  bool Valid() const { return line >= 0; }
  std::string ToString();

  // Compare two Positions and check if they are equal. Ignores the value of
  // |interesting|.
  bool operator==(const Position &o) const {
    return line == o.line && column == o.column;
  }
  bool operator<(const Position &o) const {
    if (line != o.line)
      return line < o.line;
    return column < o.column;
  }
  bool operator<=(const Position &o) const { return !(o < *this); }
};

struct Range {
  Position start;
  Position end;

  static Range FromString(const std::string &encoded);

  bool Valid() const { return start.Valid(); }
  bool Contains(int line, int column) const;
  Range RemovePrefix(Position position) const;

  std::string ToString();

  bool operator==(const Range &o) const {
    return start == o.start && end == o.end;
  }
  bool operator<(const Range &o) const {
    return !(start == o.start) ? start < o.start : end < o.end;
  }
};

// Reflection
class Reader;
class Writer;
void Reflect(Reader &visitor, Position &value);
void Reflect(Writer &visitor, Position &value);
void Reflect(Reader &visitor, Range &value);
void Reflect(Writer &visitor, Range &value);
} // namespace ccls

namespace std {
template <> struct hash<ccls::Range> {
  std::size_t operator()(ccls::Range x) const {
    union U {
      ccls::Range range = {};
      uint64_t u64;
    } u;
    static_assert(sizeof(ccls::Range) == 8);
    u.range = x;
    return hash<uint64_t>()(u.u64);
  }
};
} // namespace std

MAKE_HASHABLE(ccls::Position, t.line, t.column);
