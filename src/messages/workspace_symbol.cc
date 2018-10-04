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

#include "fuzzy_match.h"
#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"

#include <algorithm>
#include <ctype.h>
#include <functional>
#include <limits.h>

using namespace ccls;

namespace {
MethodType kMethodType = "workspace/symbol";

// Lookup |symbol| in |db| and insert the value into |result|.
bool AddSymbol(
    DB *db, WorkingFiles *working_files, SymbolIdx sym, bool use_detailed,
    std::vector<std::tuple<lsSymbolInformation, int, SymbolIdx>> *result) {
  std::optional<lsSymbolInformation> info = GetSymbolInfo(db, sym, true);
  if (!info)
    return false;

  Use loc;
  if (Maybe<DeclRef> dr = GetDefinitionSpell(db, sym))
    loc = *dr;
  else {
    auto decls = GetNonDefDeclarations(db, sym);
    if (decls.empty())
      return false;
    loc = decls[0];
  }

  std::optional<lsLocation> ls_location = GetLsLocation(db, working_files, loc);
  if (!ls_location)
    return false;
  info->location = *ls_location;
  result->emplace_back(*info, int(use_detailed), sym);
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
  void Run(In_WorkspaceSymbol *request) override {
    Out_WorkspaceSymbol out;
    out.id = request->id;

    std::string query = request->params.query;

    // {symbol info, matching detailed_name or short_name, index}
    std::vector<std::tuple<lsSymbolInformation, int, SymbolIdx>> cands;
    bool sensitive = g_config->workspaceSymbol.caseSensitivity;

    // Find subsequence matches.
    std::string query_without_space;
    query_without_space.reserve(query.size());
    for (char c : query)
      if (!isspace(c))
        query_without_space += c;

    auto Add = [&](SymbolIdx sym) {
      std::string_view detailed_name = db->GetSymbolName(sym, true);
      int pos =
          ReverseSubseqMatch(query_without_space, detailed_name, sensitive);
      return pos >= 0 &&
             AddSymbol(db, working_files, sym,
                       detailed_name.find(':', pos) != std::string::npos,
                       &cands) &&
             cands.size() >= g_config->workspaceSymbol.maxNum;
    };
    for (auto &func : db->funcs)
      if (Add({func.usr, SymbolKind::Func}))
        goto done_add;
    for (auto &type : db->types)
      if (Add({type.usr, SymbolKind::Type}))
        goto done_add;
    for (auto &var : db->vars)
      if (var.def.size() && !var.def[0].is_local() &&
          Add({var.usr, SymbolKind::Var}))
        goto done_add;
  done_add:

    if (g_config->workspaceSymbol.sort &&
        query.size() <= FuzzyMatcher::kMaxPat) {
      // Sort results with a fuzzy matching algorithm.
      int longest = 0;
      for (auto &cand : cands)
        longest = std::max(
            longest, int(db->GetSymbolName(std::get<2>(cand), true).size()));
      FuzzyMatcher fuzzy(query, g_config->workspaceSymbol.caseSensitivity);
      for (auto &cand : cands)
        std::get<1>(cand) = fuzzy.Match(
            db->GetSymbolName(std::get<2>(cand), std::get<1>(cand)));
      std::sort(cands.begin(), cands.end(), [](const auto &l, const auto &r) {
        return std::get<1>(l) > std::get<1>(r);
      });
      out.result.reserve(cands.size());
      for (auto &cand : cands) {
        // Discard awful candidates.
        if (std::get<1>(cand) <= FuzzyMatcher::kMinScore)
          break;
        out.result.push_back(std::get<0>(cand));
      }
    } else {
      out.result.reserve(cands.size());
      for (auto &cand : cands)
        out.result.push_back(std::get<0>(cand));
    }

    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_WorkspaceSymbol);
} // namespace
