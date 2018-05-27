#include "fuzzy_match.h"
#include "lex_utils.h"
#include "message_handler.h"
#include "query_utils.h"
#include "queue_manager.h"

#include <ctype.h>
#include <limits.h>
#include <algorithm>
#include <functional>

namespace {
MethodType kMethodType = "workspace/symbol";

// Lookup |symbol| in |db| and insert the value into |result|.
bool AddSymbol(
    QueryDatabase* db,
    WorkingFiles* working_files,
    int i,
    bool use_detailed,
    std::vector<std::tuple<lsSymbolInformation, bool, int>>* result) {
  SymbolIdx symbol = db->symbols[i];
  std::optional<lsSymbolInformation> info =
      GetSymbolInfo(db, working_files, symbol, true);
  if (!info)
    return false;

  Use loc;
  if (Maybe<Use> location = GetDefinitionExtent(db, symbol))
    loc = *location;
  else {
    auto decls = GetNonDefDeclarations(db, symbol);
    if (decls.empty())
      return false;
    loc = decls[0];
  }

  std::optional<lsLocation> ls_location = GetLsLocation(db, working_files, loc);
  if (!ls_location)
    return false;
  info->location = *ls_location;
  result->emplace_back(*info, use_detailed, i);
  return true;
}

struct In_WorkspaceSymbol : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct Params {
    std::string query;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(In_WorkspaceSymbol::Params, query);
MAKE_REFLECT_STRUCT(In_WorkspaceSymbol, id, params);
REGISTER_IN_MESSAGE(In_WorkspaceSymbol);

struct Out_WorkspaceSymbol : public lsOutMessage<Out_WorkspaceSymbol> {
  lsRequestId id;
  std::vector<lsSymbolInformation> result;
};
MAKE_REFLECT_STRUCT(Out_WorkspaceSymbol, jsonrpc, id, result);

///// Fuzzy matching

struct Handler_WorkspaceSymbol : BaseMessageHandler<In_WorkspaceSymbol> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_WorkspaceSymbol* request) override {
    Out_WorkspaceSymbol out;
    out.id = request->id;

    std::string query = request->params.query;

    // {symbol info, matching detailed_name or short_name, index}
    std::vector<std::tuple<lsSymbolInformation, bool, int>> unsorted;
    bool sensitive = g_config->workspaceSymbol.caseSensitivity;

    // Find subsequence matches.
    std::string query_without_space;
    query_without_space.reserve(query.size());
    for (char c : query)
      if (!isspace(c))
        query_without_space += c;

    for (int i = 0; i < (int)db->symbols.size(); ++i) {
      std::string_view detailed_name = db->GetSymbolName(i, true);
      int pos =
        ReverseSubseqMatch(query_without_space, detailed_name, sensitive);
      if (pos >= 0 &&
        AddSymbol(db, working_files, i,
          detailed_name.find(':', pos) != std::string::npos,
          &unsorted) &&
        unsorted.size() >= g_config->workspaceSymbol.maxNum)
        break;
    }

    if (g_config->workspaceSymbol.sort && query.size() <= FuzzyMatcher::kMaxPat) {
      // Sort results with a fuzzy matching algorithm.
      int longest = 0;
      for (int i = 0; i < int(unsorted.size()); i++) {
        longest = std::max(
            longest,
            int(db->GetSymbolName(std::get<2>(unsorted[i]), true).size()));
      }
      FuzzyMatcher fuzzy(query, g_config->workspaceSymbol.caseSensitivity);
      std::vector<std::pair<int, int>> permutation(unsorted.size());
      for (int i = 0; i < int(unsorted.size()); i++) {
        permutation[i] = {
            fuzzy.Match(db->GetSymbolName(std::get<2>(unsorted[i]),
                                          std::get<1>(unsorted[i]))),
            i};
      }
      std::sort(permutation.begin(), permutation.end(),
                std::greater<std::pair<int, int>>());
      out.result.reserve(unsorted.size());
      // Discard awful candidates.
      for (int i = 0; i < int(unsorted.size()) &&
                      permutation[i].first > FuzzyMatcher::kMinScore;
           i++)
        out.result.push_back(
            std::move(std::get<0>(unsorted[permutation[i].second])));
    } else {
      out.result.reserve(unsorted.size());
      for (auto& entry : unsorted)
        out.result.push_back(std::get<0>(entry));
    }

    QueueManager::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_WorkspaceSymbol);
}  // namespace
