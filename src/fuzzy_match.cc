#include "fuzzy_match.h"

#include <ctype.h>
#include <algorithm>

enum CharClass { Other, Lower, Upper };
enum CharRole { None, Tail, Head };

namespace {
CharClass GetCharClass(int c) {
  if (islower(c))
    return Lower;
  if (isupper(c))
    return Upper;
  return Other;
}

void CalculateRoles(std::string_view s, int roles[], int* class_set) {
  if (s.empty()) {
    *class_set = 0;
    return;
  }
  CharClass pre = Other, cur = GetCharClass(s[0]), suc;
  *class_set = 1 << cur;
  auto fn = [&]() {
    if (cur == Other)
      return None;
    // U(U)L is Head while U(U)U is Tail
    return pre == Other || (cur == Upper && (pre == Lower || suc != Upper))
               ? Head
               : Tail;
  };
  for (size_t i = 0; i < s.size() - 1; i++) {
    suc = GetCharClass(s[i + 1]);
    *class_set |= 1 << suc;
    roles[i] = fn();
    pre = cur;
    cur = suc;
  }
  roles[s.size() - 1] = fn();
}
}

int FuzzyMatcher::MissScore(int j, bool last) {
  int s = last ? -20 : 0;
  if (text_role[j] == Head)
    s -= 10;
  return s;
}

int FuzzyMatcher::MatchScore(int i, int j, bool last) {
  int s = 40;
  if (pat[i] == text[j]) {
    s++;
    if ((pat_set & 1 << Upper) || i == j)
      s += 20;
  }
  if (pat_role[i] == Head && text_role[j] == Head)
    s += 50;
  if (text_role[j] == Tail && i && !last)
    s -= 50;
  if (pat_role[i] == Head && text_role[j] == Tail)
    s -= 30;
  if (i == 0 && text_role[j] == Tail)
    s -= 70;
  return s;
}

FuzzyMatcher::FuzzyMatcher(std::string_view pattern) {
  CalculateRoles(pattern, pat_role, &pat_set);
  size_t n = 0;
  for (size_t i = 0; i < pattern.size(); i++)
    if (pattern[i] != ' ') {
      pat += pattern[i];
      low_pat[n] = ::tolower(pattern[i]);
      pat_role[n] = pat_role[i];
      n++;
    }
}

int FuzzyMatcher::Match(std::string_view text) {
  int n = int(text.size());
  if (n > kMaxText)
    return kMinScore + 1;
  this->text = text;
  for (int i = 0; i < n; i++)
    low_text[i] = ::tolower(text[i]);
  CalculateRoles(text, text_role, &text_set);
  dp[0][0][0] = 0;
  dp[0][0][1] = kMinScore;
  for (int j = 0; j < n; j++) {
    dp[0][j + 1][0] = dp[0][j][0] + MissScore(j, false);
    dp[0][j + 1][1] = kMinScore;
  }
  for (int i = 0; i < int(pat.size()); i++) {
    int(*pre)[2] = dp[i & 1];
    int(*cur)[2] = dp[(i + 1) & 1];
    cur[0][0] = cur[0][1] = kMinScore;
    for (int j = 0; j < n; j++) {
      cur[j + 1][0] = std::max(cur[j][0] + MissScore(j, false),
                               cur[j][1] + MissScore(j, true));
      if (low_pat[i] != low_text[j])
        cur[j + 1][1] = kMinScore;
      else {
        cur[j + 1][1] = std::max(pre[j][0] + MatchScore(i, j, false),
                                 pre[j][1] + MatchScore(i, j, true));
      }
    }
  }

  // Enumerate the end position of the match in str. Each removed trailing
  // character has a penulty.
  int ret = kMinScore;
  for (int j = 1; j <= n; j++)
    ret = std::max(ret, dp[pat.size() & 1][j][1] - 3 * (n - j));
  return ret;
}
