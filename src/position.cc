#include "position.h"

#include <stdlib.h>

Position::Position() : line(-1), column(-1) {}

Position::Position(int16_t line, int16_t column) : line(line), column(column) {}

Position::Position(const char* encoded) {
  char* p = const_cast<char*>(encoded);
  line = int16_t(strtol(p, &p, 10)) - 1;
  assert(*p == ':');
  p++;
  column = int16_t(strtol(p, &p, 10)) - 1;
}

std::string Position::ToString() {
  // Output looks like this:
  //
  //  1:2
  //
  // 1 => line
  // 2 => column

  std::string result;
  result += std::to_string(line + 1);
  result += ':';
  result += std::to_string(column + 1);
  return result;
}

std::string Position::ToPrettyString(const std::string& filename) {
  // Output looks like this:
  //
  //  1:2:3
  //
  // 1 => filename
  // 2 => line
  // 3 => column

  std::string result;
  result += filename;
  result += ':';
  result += std::to_string(line + 1);
  result += ':';
  result += std::to_string(column + 1);
  return result;
}

bool Position::operator==(const Position& that) const {
  return line == that.line && column == that.column;
}

bool Position::operator!=(const Position& that) const {
  return !(*this == that);
}

bool Position::operator<(const Position& that) const {
  if (line != that.line)
    return line < that.line;
  return column < that.column;
}

Range::Range() {}

Range::Range(Position position) : Range(position, position) {}

Range::Range(Position start, Position end) : start(start), end(end) {}

Range::Range(const char* encoded) {
  char* p = const_cast<char*>(encoded);
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
}

bool Range::Contains(int line, int column) const {
  if (line == start.line && line == end.line)
    return column >= start.column && column < end.column;
  if (line == start.line)
    return column >= start.column;
  if (line == end.line)
    return column < end.column;
  if (line > start.line && line < end.line)
    return true;
  return false;
}

Range Range::RemovePrefix(Position position) const {
  return {std::min(std::max(position, start), end), end};
}

std::string Range::ToString() {
  // Output looks like this:
  //
  //  1:2-3:4
  //
  // 1 => start line
  // 2 => start column
  // 3 => end line
  // 4 => end column

  std::string output;

  output += std::to_string(start.line + 1);
  output += ':';
  output += std::to_string(start.column + 1);
  output += '-';
  output += std::to_string(end.line + 1);
  output += ':';
  output += std::to_string(end.column + 1);

  return output;
}

bool Range::operator==(const Range& that) const {
  return start == that.start && end == that.end;
}

bool Range::operator!=(const Range& that) const {
  return !(*this == that);
}

bool Range::operator<(const Range& that) const {
  if (start != that.start)
    return start < that.start;
  return end < that.end;
}

// Position
void Reflect(Reader& visitor, Position& value) {
  if (visitor.Format() == SerializeFormat::Json) {
    std::string s = visitor.GetString();
    value = Position(s.c_str());
  } else {
    Reflect(visitor, value.line);
    Reflect(visitor, value.column);
  }
}
void Reflect(Writer& visitor, Position& value) {
  if (visitor.Format() == SerializeFormat::Json) {
    std::string output = value.ToString();
    visitor.String(output.c_str(), output.size());
  } else {
    Reflect(visitor, value.line);
    Reflect(visitor, value.column);
  }
}

// Range
void Reflect(Reader& visitor, Range& value) {
  if (visitor.Format() == SerializeFormat::Json) {
    std::string s = visitor.GetString();
    value = Range(s.c_str());
  } else {
    Reflect(visitor, value.start.line);
    Reflect(visitor, value.start.column);
    Reflect(visitor, value.end.line);
    Reflect(visitor, value.end.column);
  }
}
void Reflect(Writer& visitor, Range& value) {
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
