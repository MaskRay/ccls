// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "utils.hh"

#include <stdint.h>
#include <string>

namespace ccls {
struct Pos {
  int16_t line = -1;
  int16_t column = -1;

  static Pos FromString(const std::string &encoded);

  bool Valid() const { return line >= 0; }
  std::string ToString();

  // Compare two Positions and check if they are equal. Ignores the value of
  // |interesting|.
  bool operator==(const Pos &o) const {
    return line == o.line && column == o.column;
  }
  bool operator<(const Pos &o) const {
    if (line != o.line)
      return line < o.line;
    return column < o.column;
  }
  bool operator<=(const Pos &o) const { return !(o < *this); }
};

struct Range {
  Pos start;
  Pos end;

  static Range FromString(const std::string &encoded);

  bool Valid() const { return start.Valid(); }
  bool Contains(int line, int column) const;

  std::string ToString();

  bool operator==(const Range &o) const {
    return start == o.start && end == o.end;
  }
  bool operator<(const Range &o) const {
    return !(start == o.start) ? start < o.start : end < o.end;
  }
};

// Reflection
struct JsonReader;
struct JsonWriter;
struct BinaryReader;
struct BinaryWriter;

void Reflect(JsonReader &visitor, Pos &value);
void Reflect(JsonReader &visitor, Range &value);
void Reflect(JsonWriter &visitor, Pos &value);
void Reflect(JsonWriter &visitor, Range &value);
void Reflect(BinaryReader &visitor, Pos &value);
void Reflect(BinaryReader &visitor, Range &value);
void Reflect(BinaryWriter &visitor, Pos &value);
void Reflect(BinaryWriter &visitor, Range &value);
} // namespace ccls

namespace std {
template <> struct hash<ccls::Range> {
  std::size_t operator()(ccls::Range x) const {
    union {
      ccls::Range range;
      uint64_t u64;
    } u{x};
    static_assert(sizeof(ccls::Range) == 8);
    return hash<uint64_t>()(u.u64);
  }
};
} // namespace std

MAKE_HASHABLE(ccls::Pos, t.line, t.column);
