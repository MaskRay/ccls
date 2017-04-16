#include "fuzzy.h"

#include <doctest/doctest.h>

Matcher::Matcher(const std::string& search) {
  std::string real_search;
  real_search.reserve(search.size() * 3 + 2);
  for (auto c : search) {
    real_search += ".*";
    real_search += c;
  }
  real_search += ".*";

  regex = std::regex(real_search,
      std::regex_constants::ECMAScript |
      std::regex_constants::icase |
      std::regex_constants::optimize
      //std::regex_constants::nosubs
  );
}

bool Matcher::IsMatch(const std::string& value) {
  //std::smatch match;
  //return std::regex_match(value, match, regex);
  return std::regex_match(value, regex, std::regex_constants::match_any);
}

TEST_SUITE("Matcher");

TEST_CASE("sanity") {
  Matcher m("abc");
  // TODO: check case
  CHECK(m.IsMatch("abc"));
  CHECK(m.IsMatch("fooabc"));
  CHECK(m.IsMatch("abc"));
  CHECK(m.IsMatch("abcfoo"));
  CHECK(m.IsMatch("11a11b11c11"));
}

TEST_SUITE_END();
