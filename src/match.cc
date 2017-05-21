#include "match.h"

#include <doctest/doctest.h>

Matcher::Matcher(const std::string& search) {
  /*
  std::string real_search;
  real_search.reserve(search.size() * 3 + 2);
  for (auto c : search) {
    real_search += ".*";
    real_search += c;
  }
  real_search += ".*";
  */

  regex_string = search;
  regex = std::regex(regex_string,
      std::regex_constants::ECMAScript |
      std::regex_constants::icase |
      std::regex_constants::optimize
      //std::regex_constants::nosubs
  );
}

bool Matcher::IsMatch(const std::string& value) const {
  //std::smatch match;
  //return std::regex_match(value, match, regex);
  return std::regex_match(value, regex, std::regex_constants::match_any);
}

GroupMatch::GroupMatch(
    const std::vector<std::string>& whitelist,
    const std::vector<std::string>& blacklist) {
  for (const std::string& entry : whitelist)
    this->whitelist.push_back(Matcher(entry));
  for (const std::string& entry : blacklist)
    this->blacklist.push_back(Matcher(entry));
}

bool GroupMatch::IsMatch(const std::string& value, std::string* match_failure_reason) const {
  for (const Matcher& m : whitelist) {
    if (!m.IsMatch(value)) {
      if (match_failure_reason)
        *match_failure_reason = "whitelist \"" + m.regex_string + "\"";
      return false;
    }
  }

  for (const Matcher& m : blacklist) {
    if (m.IsMatch(value)) {
      if (match_failure_reason)
        *match_failure_reason = "blacklist \"" + m.regex_string + "\"";
      return false;
    }
  }

  return true;
}

  
TEST_SUITE("Matcher");

TEST_CASE("sanity") {
  Matcher m("abc");
  // TODO: check case
  //CHECK(m.IsMatch("abc"));
  //CHECK(m.IsMatch("fooabc"));
  //CHECK(m.IsMatch("abc"));
  //CHECK(m.IsMatch("abcfoo"));
  //CHECK(m.IsMatch("11a11b11c11"));
}

TEST_SUITE_END();
