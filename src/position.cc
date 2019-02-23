// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "position.hh"

#include "serializer.hh"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

namespace ccls {
Pos Pos::FromString(const std::string &encoded) {
  char *p = const_cast<char *>(encoded.c_str());
  uint16_t line = uint16_t(strtoul(p, &p, 10) - 1);
  assert(*p == ':');
  p++;
  int16_t column = int16_t(strtol(p, &p, 10)) - 1;
  return {line, column};
}

std::string Pos::ToString() {
  char buf[99];
  snprintf(buf, sizeof buf, "%d:%d", line + 1, column + 1);
  return buf;
}

Range Range::FromString(const std::string &encoded) {
  Pos start, end;
  char *p = const_cast<char *>(encoded.c_str());
  start.line = uint16_t(strtoul(p, &p, 10) - 1);
  assert(*p == ':');
  p++;
  start.column = int16_t(strtol(p, &p, 10)) - 1;
  assert(*p == '-');
  p++;

  end.line = uint16_t(strtoul(p, &p, 10) - 1);
  assert(*p == ':');
  p++;
  end.column = int16_t(strtol(p, nullptr, 10)) - 1;
  return {start, end};
}

bool Range::Contains(int line, int column) const {
  if (line > INT16_MAX)
    return false;
  Pos p{(uint16_t)line, (int16_t)std::min<int>(column, INT16_MAX)};
  return !(p < start) && p < end;
}

std::string Range::ToString() {
  char buf[99];
  snprintf(buf, sizeof buf, "%d:%d-%d:%d", start.line + 1, start.column + 1,
           end.line + 1, end.column + 1);
  return buf;
}

void Reflect(JsonReader &vis, Pos &v) { v = Pos::FromString(vis.GetString()); }
void Reflect(JsonReader &vis, Range &v) {
  v = Range::FromString(vis.GetString());
}

void Reflect(JsonWriter &vis, Pos &v) {
  std::string output = v.ToString();
  vis.String(output.c_str(), output.size());
}
void Reflect(JsonWriter &vis, Range &v) {
  std::string output = v.ToString();
  vis.String(output.c_str(), output.size());
}

void Reflect(BinaryReader &visitor, Pos &value) {
  Reflect(visitor, value.line);
  Reflect(visitor, value.column);
}
void Reflect(BinaryReader &visitor, Range &value) {
  Reflect(visitor, value.start.line);
  Reflect(visitor, value.start.column);
  Reflect(visitor, value.end.line);
  Reflect(visitor, value.end.column);
}

void Reflect(BinaryWriter &vis, Pos &v) {
  Reflect(vis, v.line);
  Reflect(vis, v.column);
}
void Reflect(BinaryWriter &vis, Range &v) {
  Reflect(vis, v.start.line);
  Reflect(vis, v.start.column);
  Reflect(vis, v.end.line);
  Reflect(vis, v.end.column);
}
} // namespace ccls
