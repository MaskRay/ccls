#include "lex_utils.h"
#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

#include <loguru.hpp>

#include <ctype.h>
#include <limits.h>
#include <algorithm>
#include <functional>

namespace {

// Lookup |symbol| in |db| and insert the value into |result|.
bool InsertSymbolIntoResult(QueryDatabase* db,
                            WorkingFiles* working_files,
                            SymbolIdx symbol,
                            std::vector<lsSymbolInformation>* result) {
  optional<lsSymbolInformation> info =
      GetSymbolInfo(db, working_files, symbol, false /*use_short_name*/);
  if (!info)
    return false;

  optional<QueryLocation> location = GetDefinitionExtentOfSymbol(db, symbol);
  if (!location) {
    auto decls = GetDeclarationsOfSymbolForGotoDefinition(db, symbol);
    if (decls.empty())
      return false;
    location = decls[0];
  }

  optional<lsLocation> ls_location =
      GetLsLocation(db, working_files, *location);
  if (!ls_location)
    return false;
  info->location = *ls_location;
  result->push_back(*info);
  return true;
}

struct Ipc_WorkspaceSymbol : public RequestMessage<Ipc_WorkspaceSymbol> {
  const static IpcId kIpcId = IpcId::WorkspaceSymbol;
  struct Params {
    std::string query;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_WorkspaceSymbol::Params, query);
MAKE_REFLECT_STRUCT(Ipc_WorkspaceSymbol, id, params);
REGISTER_IPC_MESSAGE(Ipc_WorkspaceSymbol);

struct Out_WorkspaceSymbol : public lsOutMessage<Out_WorkspaceSymbol> {
  lsRequestId id;
  std::vector<lsSymbolInformation> result;
};
MAKE_REFLECT_STRUCT(Out_WorkspaceSymbol, jsonrpc, id, result);

///// Fuzzy matching

// Negative but far from INT_MIN so that intermediate results are hard to
// overflow
constexpr int kMinScore = INT_MIN / 2;
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
int FuzzyEvaluate(const std::string& pattern,
                  const std::string& str,
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

struct WorkspaceSymbolHandler : BaseMessageHandler<Ipc_WorkspaceSymbol> {
  void Run(Ipc_WorkspaceSymbol* request) override {
    Out_WorkspaceSymbol out;
    out.id = request->id;

    LOG_S(INFO) << "[querydb] Considering " << db->detailed_names.size()
                << " candidates for query " << request->params.query;

    std::string query = request->params.query;

    std::unordered_set<std::string> inserted_results;
    // db->detailed_names indices of each lsSymbolInformation in out.result
    std::vector<int> result_indices;
    std::vector<lsSymbolInformation> unsorted_results;
    inserted_results.reserve(config->maxWorkspaceSearchResults);
    result_indices.reserve(config->maxWorkspaceSearchResults);

    // We use detailed_names for exact matches and short_names for fuzzy matches
    // because otherwise the fuzzy match is likely to match on parameter names
    // and the like.
    // TODO: make detailed_names not include function parameter information (or
    // introduce additional metadata) so that we can do fuzzy search with
    // detailed_names.

    // Find exact substring matches.
    for (int i = 0; i < db->detailed_names.size(); ++i) {
      if (db->detailed_names[i].find(query) != std::string::npos) {
        // Do not show the same entry twice.
        if (!inserted_results.insert(db->detailed_names[i]).second)
          continue;

        if (InsertSymbolIntoResult(db, working_files, db->symbols[i],
                                   &unsorted_results)) {
          result_indices.push_back(i);
          if (unsorted_results.size() >= config->maxWorkspaceSearchResults)
            break;
        }
      }
    }

    // Find subsequence matches.
    if (unsorted_results.size() < config->maxWorkspaceSearchResults) {
      std::string query_without_space;
      query_without_space.reserve(query.size());
      for (char c : query)
        if (!isspace(c))
          query_without_space += c;

      for (int i = 0; i < db->short_names.size(); ++i) {
        if (SubstringMatch(query_without_space, db->short_names[i])) {
          // Do not show the same entry twice.
          if (!inserted_results.insert(db->detailed_names[i]).second)
            continue;

          if (InsertSymbolIntoResult(db, working_files, db->symbols[i],
                                     &unsorted_results)) {
            result_indices.push_back(i);
            if (unsorted_results.size() >= config->maxWorkspaceSearchResults)
              break;
          }
        }
      }
    }

    if (config->sortWorkspaceSearchResults) {
      // Sort results with a fuzzy matching algorithm.
      int longest = 0;
      for (int i : result_indices)
        longest = std::max(longest, int(db->short_names[i].size()));

      std::vector<int> score(longest);  // score for each position
      std::vector<int> dp(
          longest);  // dp[i]: maximum value by aligning pattern to str[0..i]
      std::vector<std::pair<int, int>> permutation(result_indices.size());
      for (int i = 0; i < int(result_indices.size()); i++) {
        permutation[i] = {
            FuzzyEvaluate(query, db->short_names[result_indices[i]], score, dp),
            i};
      }
      std::sort(permutation.begin(), permutation.end(),
                std::greater<std::pair<int, int>>());
      out.result.reserve(result_indices.size());
      for (int i = 0; i < int(result_indices.size()); i++)
        out.result.push_back(
            std::move(unsorted_results[permutation[i].second]));
    } else {
      out.result.reserve(unsorted_results.size());
      for (const auto& entry : unsorted_results)
        out.result.push_back(std::move(entry));
    }

    LOG_S(INFO) << "[querydb] Found " << out.result.size()
                << " results for query " << query;
    QueueManager::WriteStdout(IpcId::WorkspaceSymbol, out);
  }
};
REGISTER_MESSAGE_HANDLER(WorkspaceSymbolHandler);
}  // namespace
