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

#include "match.hh"

#include "lsp.hh"
#include "pipeline.hh"

namespace ccls {
std::optional<Matcher> Matcher::Create(const std::string &search) {
  try {
    Matcher m;
    m.regex_string = search;
    m.regex = std::regex(search, std::regex_constants::ECMAScript |
                                     std::regex_constants::icase |
                                     std::regex_constants::optimize
                         // std::regex_constants::nosubs
    );
    return m;
  } catch (const std::exception &e) {
    lsShowMessageParams params;
    params.type = MessageType::Error;
    params.message =
        "failed to parse EMCAScript regex " + search + " : " + e.what();
    pipeline::Notify(window_showMessage, params);
    return std::nullopt;
  }
}

bool Matcher::IsMatch(const std::string &value) const {
  // std::smatch match;
  // return std::regex_match(value, match, regex);
  return std::regex_search(value, regex, std::regex_constants::match_any);
}

GroupMatch::GroupMatch(const std::vector<std::string> &whitelist,
                       const std::vector<std::string> &blacklist) {
  for (const std::string &entry : whitelist) {
    std::optional<Matcher> m = Matcher::Create(entry);
    if (m)
      this->whitelist.push_back(*m);
  }
  for (const std::string &entry : blacklist) {
    std::optional<Matcher> m = Matcher::Create(entry);
    if (m)
      this->blacklist.push_back(*m);
  }
}

bool GroupMatch::IsMatch(const std::string &value,
                         std::string *match_failure_reason) const {
  for (const Matcher &m : whitelist)
    if (m.IsMatch(value))
      return true;

  for (const Matcher &m : blacklist)
    if (m.IsMatch(value)) {
      if (match_failure_reason)
        *match_failure_reason = "blacklist \"" + m.regex_string + "\"";
      return false;
    }

  return true;
}
} // namespace ccls
