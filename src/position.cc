/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "position.hh"

#include "serializer.hh"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

namespace ccls {
Pos Pos::FromString(const std::string &encoded) {
  char *p = const_cast<char *>(encoded.c_str());
  int16_t line = int16_t(strtol(p, &p, 10)) - 1;
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
  start.line = int16_t(strtol(p, &p, 10)) - 1;
  assert(*p == ':');
  p++;
  start.column = int16_t(strtol(p, &p, 10)) - 1;
  assert(*p == '-');
  p++;

  end.line = int16_t(strtol(p, &p, 10)) - 1;
  assert(*p == ':');
  p++;
  end.column = int16_t(strtol(p, nullptr, 10)) - 1;
  return {start, end};
}

bool Range::Contains(int line, int column) const {
  if (line > INT16_MAX)
    return false;
  Pos p{int16_t(line), int16_t(std::min(column, INT16_MAX))};
  return !(p < start) && p < end;
}

Range Range::RemovePrefix(Pos position) const {
  return {std::min(std::max(position, start), end), end};
}

std::string Range::ToString() {
  char buf[99];
  snprintf(buf, sizeof buf, "%d:%d-%d:%d", start.line + 1, start.column + 1,
           end.line + 1, end.column + 1);
  return buf;
}

// Position
void Reflect(Reader &visitor, Pos &value) {
  if (visitor.Format() == SerializeFormat::Json) {
    value = Pos::FromString(visitor.GetString());
  } else {
    Reflect(visitor, value.line);
    Reflect(visitor, value.column);
  }
}
void Reflect(Writer &visitor, Pos &value) {
  if (visitor.Format() == SerializeFormat::Json) {
    std::string output = value.ToString();
    visitor.String(output.c_str(), output.size());
  } else {
    Reflect(visitor, value.line);
    Reflect(visitor, value.column);
  }
}

// Range
void Reflect(Reader &visitor, Range &value) {
  if (visitor.Format() == SerializeFormat::Json) {
    value = Range::FromString(visitor.GetString());
  } else {
    Reflect(visitor, value.start.line);
    Reflect(visitor, value.start.column);
    Reflect(visitor, value.end.line);
    Reflect(visitor, value.end.column);
  }
}
void Reflect(Writer &visitor, Range &value) {
  if (visitor.Format() == SerializeFormat::Json) {
    std::string output = value.ToString();
    visitor.String(output.c_str(), output.size());
  } else {
    Reflect(visitor, value.start.line);
    Reflect(visitor, value.start.column);
    Reflect(visitor, value.end.line);
    Reflect(visitor, value.end.column);
  }
}
} // namespace ccls
