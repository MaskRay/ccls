#include "fuzzy_match.h"

#include <ctype.h>
#include <limits.h>
#include <algorithm>

// Penalty of dropping a leading character in str
constexpr int kLeadingGapScore = -4;
// Penalty of dropping a non-leading character in str
constexpr int kGapScore = -5;
// Bonus of aligning with an initial character of a word in pattern. Must be
// greater than 1
constexpr int kPatternStartMultiplier = 2;

constexpr int kWordStartScore = 50;
constexpr int kNonWordScore = 40;
constexpr int kCaseMatchScore = 2;

// Less than kWordStartScore
constexpr int kConsecutiveScore = kWordStartScore + kGapScore;
// Slightly less than kConsecutiveScore
constexpr int kCamelScore = kWordStartScore + kGapScore - 1;

enum class CharClass { Lower, Upper, Digit, NonWord };

static CharClass GetCharClass(int c) {
  if (islower(c))
    return CharClass::Lower;
  if (isupper(c))
    return CharClass::Upper;
  if (isdigit(c))
    return CharClass::Digit;
  return CharClass::NonWord;
}

static int GetScoreFor(CharClass prev, CharClass curr) {
  if (prev == CharClass::NonWord && curr != CharClass::NonWord)
    return kWordStartScore;
  if ((prev == CharClass::Lower && curr == CharClass::Upper) ||
      (prev != CharClass::Digit && curr == CharClass::Digit))
    return kCamelScore;
  if (curr == CharClass::NonWord)
    return kNonWordScore;
  return 0;
}

/*
fuzzyEvaluate implements a global sequence alignment algorithm to find the
maximum accumulated score by aligning `pattern` to `str`. It applies when
`pattern` is a subsequence of `str`.

Scoring criteria
- Prefer matches at the start of a word, or the start of subwords in
CamelCase/camelCase/camel123 words. See kWordStartScore/kCamelScore
- Non-word characters matter. See kNonWordScore
- The first characters of words of `pattern` receive bonus because they usually
have more significance than the rest. See kPatternStartMultiplier
- Superfluous characters in `str` will reduce the score (gap penalty). See
kGapScore
- Prefer early occurrence of the first character. See kLeadingGapScore/kGapScore

The recurrence of the dynamic programming:
dp[i][j]: maximum accumulated score by aligning pattern[0..i] to str[0..j]
dp[0][j] = leading_gap_penalty(0, j) + score[j]
dp[i][j] = max(dp[i-1][j-1] + CONSECUTIVE_SCORE, max(dp[i-1][k] +
gap_penalty(k+1, j) + score[j] : k < j))
The first dimension can be suppressed since we do not need a matching scheme,
which reduces the space complexity from O(N*M) to O(M)
*/
int FuzzyEvaluate(std::string_view pattern,
                  std::string_view str,
                  std::vector<int>& score,
                  std::vector<int>& dp) {
  bool pfirst = true,  // aligning the first character of pattern
      pstart = true;   // whether we are aligning the start of a word in pattern
  int uleft = 0,       // value of the upper left cell
      ulefts = 0,      // maximum value of uleft and cells on the left
      left, lefts;     // similar to uleft/ulefts, but for the next row

  // Calculate position score for each character in str.
  CharClass prev = CharClass::NonWord;
  for (int i = 0; i < int(str.size()); i++) {
    CharClass cur = GetCharClass(str[i]);
    score[i] = GetScoreFor(prev, cur);
    prev = cur;
  }
  std::fill_n(dp.begin(), str.size(), kMinScore);

  // Align each character of pattern.
  for (unsigned char pc : pattern) {
    if (isspace(pc)) {
      pstart = true;
      continue;
    }
    lefts = kMinScore;
    // Enumerate the character in str to be aligned with pc.
    for (int i = 0; i < int(str.size()); i++) {
      left = dp[i];
      lefts = std::max(lefts + kGapScore, left);
      // Use lower() if case-insensitive
      if (tolower(pc) == tolower(str[i])) {
        int t = score[i] * (pstart ? kPatternStartMultiplier : 1);
        dp[i] = (pfirst ? kLeadingGapScore * i + t
                        : std::max(uleft + kConsecutiveScore, ulefts + t)) +
                (pc == str[i] ? kCaseMatchScore : 0);
      } else
        dp[i] = kMinScore;
      uleft = left;
      ulefts = lefts;
    }
    pfirst = pstart = false;
  }

  // Enumerate the end position of the match in str. Each removed trailing
  // character has a penulty of kGapScore.
  lefts = kMinScore;
  for (int i = 0; i < int(str.size()); i++)
    lefts = std::max(lefts + kGapScore, dp[i]);
  return lefts;
}
