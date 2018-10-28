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

#pragma once

#include <optional>

#include <regex>
#include <string>
#include <vector>

namespace ccls {
struct Matcher {
  static std::optional<Matcher> Create(const std::string &search);

  bool IsMatch(const std::string &value) const;

  std::string regex_string;
  std::regex regex;
};

// Check multiple |Matcher| instances at the same time.
struct GroupMatch {
  GroupMatch(const std::vector<std::string> &whitelist,
             const std::vector<std::string> &blacklist);

  bool IsMatch(const std::string &value,
               std::string *match_failure_reason = nullptr) const;

  std::vector<Matcher> whitelist;
  std::vector<Matcher> blacklist;
};
} // namespace ccls
