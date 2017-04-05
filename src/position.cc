#include "position.h"

Position::Position() {}

Position::Position(bool interesting, int32_t line, int32_t column)
  : interesting(interesting), line(line), column(column) {}

Position::Position(const char* encoded) {
  if (*encoded == '*') {
    interesting = true;
    ++encoded;
  }

  assert(encoded);
  line = atoi(encoded);
  while (*encoded && *encoded != ':')
    ++encoded;
  if (*encoded == ':')
    ++encoded;

  assert(encoded);
  column = atoi(encoded);
}

std::string Position::ToString() {
  // Output looks like this:
  //
  //  *1:2
  //
  // * => interesting
  // 1 => line
  // 2 => column

  std::string result;
  if (interesting)
    result += '*';
  result += std::to_string(line);
  result += ':';
  result += std::to_string(column);
  return result;
}

std::string Position::ToPrettyString(const std::string& filename) {
  // Output looks like this:
  //
  //  *1:2
  //
  // * => interesting
  // 1 => line
  // 2 => column

  std::string result;
  if (interesting)
    result += '*';
  result += filename;
  result += ':';
  result += std::to_string(line);
  result += ':';
  result += std::to_string(column);
  return result;
}

Position Position::WithInteresting(bool interesting) {
  Position result = *this;
  result.interesting = interesting;
  return result;
}

bool Position::operator==(const Position& that) const {
  return line == that.line && column == that.column;
}

bool Position::operator!=(const Position& that) const { return !(*this == that); }

bool Position::operator<(const Position& that) const {
  return interesting < that.interesting && line < that.line && column < that.column;
}

Range::Range() {}

Range::Range(Position start, Position end) : start(start), end(end) {}

Range::Range(const char* encoded) : start(encoded) {
  end = start;
  /*
  assert(encoded);
  while (*encoded && *encoded != '-')
    ++encoded;
  if (*encoded == '-')
    ++encoded;
  end.line = atoi(encoded);

  assert(encoded);
  while (*encoded && *encoded != ':')
    ++encoded;
  if (*encoded == ':')
    ++encoded;
  end.column = atoi(encoded);
  */
}

std::string Range::ToString() {
  std::string output;
  output += start.ToString();
  /*
  output += "-";
  output += std::to_string(end.line);
  output += ":";
  output += std::to_string(end.column);
  */
  return output;
}

Range Range::WithInteresting(bool interesting) {
  return Range(start.WithInteresting(interesting), end.WithInteresting(interesting));
}

bool Range::operator==(const Range& that) const {
  return start == that.start && end == that.end;
}

bool Range::operator!=(const Range& that) const { return !(*this == that); }

bool Range::operator<(const Range& that) const {
  return start < that.start;// || end < that.end;
}