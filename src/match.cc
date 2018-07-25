#include "match.h"

#include "lsp.h"
#include "pipeline.hh"
using namespace ccls;

// static
std::optional<Matcher> Matcher::Create(const std::string& search) {
  /*
  std::string real_search;
  real_search.reserve(search.size() * 3 + 2);
  for (auto c : search) {
    real_search += ".*";
    real_search += c;
  }
  real_search += ".*";
  */

  try {
    Matcher m;
    m.regex_string = search;
    m.regex = std::regex(
        search, std::regex_constants::ECMAScript | std::regex_constants::icase |
                    std::regex_constants::optimize
        // std::regex_constants::nosubs
    );
    return m;
  } catch (const std::exception& e) {
    Out_ShowLogMessage out;
    out.display_type = Out_ShowLogMessage::DisplayType::Show;
    out.params.type = lsMessageType::Error;
    out.params.message = "ccls: Parsing EMCAScript regex \"" + search +
                         "\" failed; " + e.what();
    pipeline::WriteStdout(kMethodType_Unknown, out);
    return std::nullopt;
  }
}

bool Matcher::IsMatch(const std::string& value) const {
  // std::smatch match;
  // return std::regex_match(value, match, regex);
  return std::regex_search(value, regex, std::regex_constants::match_any);
}

GroupMatch::GroupMatch(const std::vector<std::string>& whitelist,
                       const std::vector<std::string>& blacklist) {
  for (const std::string& entry : whitelist) {
    std::optional<Matcher> m = Matcher::Create(entry);
    if (m)
      this->whitelist.push_back(*m);
  }
  for (const std::string& entry : blacklist) {
    std::optional<Matcher> m = Matcher::Create(entry);
    if (m)
      this->blacklist.push_back(*m);
  }
}

bool GroupMatch::IsMatch(const std::string& value,
                         std::string* match_failure_reason) const {
  for (const Matcher& m : whitelist) {
    if (m.IsMatch(value))
      return true;
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
