#include "fuzzy_match.h"
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

  Maybe<Use> location = GetDefinitionExtentOfSymbol(db, symbol);
  Use loc;
  if (location)
    loc = *location;
  else {
    auto decls = GetDeclarationsOfSymbolForGotoDefinition(db, symbol);
    if (decls.empty())
      return false;
    loc = decls[0];
  }

  optional<lsLocation> ls_location = GetLsLocation(db, working_files, loc);
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


struct WorkspaceSymbolHandler : BaseMessageHandler<Ipc_WorkspaceSymbol> {
  void Run(Ipc_WorkspaceSymbol* request) override {
    Out_WorkspaceSymbol out;
    out.id = request->id;

    LOG_S(INFO) << "[querydb] Considering " << db->symbols.size()
                << " candidates for query " << request->params.query;

    std::string query = request->params.query;

    std::unordered_set<std::string> inserted_results;
    // db->detailed_names indices of each lsSymbolInformation in out.result
    std::vector<int> result_indices;
    std::vector<lsSymbolInformation> unsorted_results;
    inserted_results.reserve(config->maxWorkspaceSearchResults);
    result_indices.reserve(config->maxWorkspaceSearchResults);

    // We use detailed_names without parameters for matching.

    // Find exact substring matches.
    for (int i = 0; i < db->symbols.size(); ++i) {
      std::string_view detailed_name = db->GetSymbolDetailedName(i);
      if (detailed_name.find(query) != std::string::npos) {
        // Do not show the same entry twice.
        if (!inserted_results.insert(std::string(detailed_name)).second)
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

      for (int i = 0; i < (int)db->symbols.size(); ++i) {
        std::string_view detailed_name = db->GetSymbolDetailedName(i);
        if (SubsequenceMatch(query_without_space, detailed_name)) {
          // Do not show the same entry twice.
          if (!inserted_results.insert(std::string(detailed_name)).second)
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
        longest = std::max(longest, int(db->GetSymbolDetailedName(i).size()));

      std::vector<int> score(longest);  // score for each position
      std::vector<int> dp(
          longest);  // dp[i]: maximum value by aligning pattern to str[0..i]
      std::vector<std::pair<int, int>> permutation(result_indices.size());
      for (int i = 0; i < int(result_indices.size()); i++) {
        permutation[i] = {
          FuzzyEvaluate(query, db->GetSymbolDetailedName(result_indices[i]), score, dp),
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
