#pragma once

#include <optional.h>

#include <regex>
#include <string>
#include <vector>

using std::experimental::nullopt;
using std::experimental::optional;

struct Matcher {
  static optional<Matcher> Create(const std::string& search);

  bool IsMatch(const std::string& value) const;

  std::string regex_string;
  std::regex regex;
};

// Check multiple |Matcher| instances at the same time.
struct GroupMatch {
  GroupMatch(const std::vector<std::string>& whitelist,
             const std::vector<std::string>& blacklist);

  bool IsMatch(const std::string& value,
               std::string* match_failure_reason = nullptr) const;

  std::vector<Matcher> whitelist;
  std::vector<Matcher> blacklist;
};