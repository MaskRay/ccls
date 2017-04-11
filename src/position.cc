#include "position.h"

namespace {
// Skips until the character immediately following |skip_after|.
const char* SkipAfter(const char* input, char skip_after) {
  while (*input && *input != skip_after)
    ++input;
  ++input;
  return input;
}
}  // namespace

Position::Position() {}

Position::Position(int32_t line, int32_t column)
  : line(line), column(column) {}

Position::Position(const char* encoded) {
  assert(encoded);
  line = atoi(encoded);

  encoded = SkipAfter(encoded, ':');
  assert(encoded);
  column = atoi(encoded);
}

std::string Position::ToString() {
  // Output looks like this:
  //
  //  1:2
  //
  // 1 => line
  // 2 => column

  std::string result;
  result += std::to_string(line);
  result += ':';
  result += std::to_string(column);
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
  result += std::to_string(line);
  result += ':';
  result += std::to_string(column);
  return result;
}

bool Position::operator==(const Position& that) const {
  return line == that.line && column == that.column;
}

bool Position::operator!=(const Position& that) const { return !(*this == that); }

bool Position::operator<(const Position& that) const {
  return line < that.line && column < that.column;
}

Range::Range() {}

Range::Range(bool interesting, Position start, Position end) : interesting(interesting), start(start), end(end) {}

Range::Range(const char* encoded) {
  end = start;

  if (*encoded == '*') {
    interesting = true;
    ++encoded;
  }

  start.line = atoi(encoded);

  encoded = SkipAfter(encoded, ':');
  assert(encoded);
  start.column = atoi(encoded);

  encoded = SkipAfter(encoded, '-');
  assert(encoded);
  end.line = atoi(encoded);

  encoded = SkipAfter(encoded, ':');
  assert(encoded);
  end.column = atoi(encoded);
}

bool Range::Contains(int line, int column) const {
  if (line == start.line && line == end.line)
    return column >= start.column && column <= end.column;
  if (line == start.line)
    return column >= start.column;
  if (line == end.line)
    return column <= end.column;
  if (line > start.line && line < end.line)
    return true;
  return false;
}

std::string Range::ToString() {
  // Output looks like this:
  //
  //  *1:2-3:4
  //
  // * => if present, range is interesting
  // 1 => start line
  // 2 => start column
  // 3 => end line
  // 4 => end column

  std::string output;

  if (interesting)
    output += '*';
  output += std::to_string(start.line);
  output += ':';
  output += std::to_string(start.column);
  output += '-';
  output += std::to_string(end.line);
  output += ':';
  output += std::to_string(end.column);

  return output;
}

Range Range::WithInteresting(bool interesting) {
  Range result = *this;
  result.interesting = interesting;
  return result;
}

bool Range::operator==(const Range& that) const {
  return start == that.start && end == that.end;
}

bool Range::operator!=(const Range& that) const { return !(*this == that); }

bool Range::operator<(const Range& that) const {
  return start < that.start;// || end < that.end;
}