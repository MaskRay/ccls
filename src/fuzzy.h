#pragma once

#include <regex>

struct Matcher {
  Matcher(const std::string& search);

  bool IsMatch(const std::string& value) const;

  std::string regex_string;
  std::regex regex;
};
