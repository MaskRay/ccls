// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "utils.hh"

#include <stdint.h>
#include <string>

namespace ccls {
struct Pos {
  uint16_t line = 0;
  int16_t column = -1;

  static Pos fromString(const std::string &encoded);

  bool valid() const { return column >= 0; }
  std::string toString();

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

  static Range fromString(const std::string &encoded);

  bool valid() const { return start.valid(); }
  bool contains(int line, int column) const;

  std::string toString();

  bool operator==(const Range &o) const {
    return start == o.start && end == o.end;
  }
  bool operator<(const Range &o) const {
    return !(start == o.start) ? start < o.start : end < o.end;
  }
};

// reflection
struct JsonReader;
struct JsonWriter;
struct BinaryReader;
struct BinaryWriter;

void reflect(JsonReader &visitor, Pos &value);
void reflect(JsonReader &visitor, Range &value);
void reflect(JsonWriter &visitor, Pos &value);
void reflect(JsonWriter &visitor, Range &value);
void reflect(BinaryReader &visitor, Pos &value);
void reflect(BinaryReader &visitor, Range &value);
void reflect(BinaryWriter &visitor, Pos &value);
void reflect(BinaryWriter &visitor, Range &value);
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
