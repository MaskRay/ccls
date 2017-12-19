#include "lex_utils.h"
#include "message_handler.h"
#include "query_utils.h"

#include <loguru.hpp>

namespace {

// Lookup |symbol| in |db| and insert the value into |result|.
void InsertSymbolIntoResult(QueryDatabase* db,
                            WorkingFiles* working_files,
                            SymbolIdx symbol,
                            std::vector<lsSymbolInformation>* result) {
  optional<lsSymbolInformation> info = GetSymbolInfo(db, working_files, symbol, false /*use_short_name*/);
  if (!info)
    return;

  optional<QueryLocation> location = GetDefinitionExtentOfSymbol(db, symbol);
  if (!location) {
    auto decls = GetDeclarationsOfSymbolForGotoDefinition(db, symbol);
    if (decls.empty())
      return;
    location = decls[0];
  }

  optional<lsLocation> ls_location =
      GetLsLocation(db, working_files, *location);
  if (!ls_location)
    return;
  info->location = *ls_location;
  result->push_back(*info);
}

struct lsWorkspaceSymbolParams {
  std::string query;
};
MAKE_REFLECT_STRUCT(lsWorkspaceSymbolParams, query);

struct Ipc_WorkspaceSymbol : public IpcMessage<Ipc_WorkspaceSymbol> {
  const static IpcId kIpcId = IpcId::WorkspaceSymbol;
  lsRequestId id;
  lsWorkspaceSymbolParams params;
};
MAKE_REFLECT_STRUCT(Ipc_WorkspaceSymbol, id, params);
REGISTER_IPC_MESSAGE(Ipc_WorkspaceSymbol);

struct Out_WorkspaceSymbol : public lsOutMessage<Out_WorkspaceSymbol> {
  lsRequestId id;
  std::vector<lsSymbolInformation> result;
};
MAKE_REFLECT_STRUCT(Out_WorkspaceSymbol, jsonrpc, id, result);

struct WorkspaceSymbolHandler : BaseMessageHandler<Ipc_WorkspaceSymbol> {
  void Run(Ipc_WorkspaceSymbol* request) override {
    // TODO: implement fuzzy search, see
    // https://github.com/junegunn/fzf/blob/master/src/matcher.go for
    // inspiration

    Out_WorkspaceSymbol out;
    out.id = request->id;

    LOG_S(INFO) << "[querydb] Considering " << db->detailed_names.size()
                << " candidates for query " << request->params.query;

    std::string query = request->params.query;

    std::unordered_set<std::string> inserted_results;
    inserted_results.reserve(config->maxWorkspaceSearchResults);

    for (int i = 0; i < db->detailed_names.size(); ++i) {
      if (db->detailed_names[i].find(query) != std::string::npos) {
        // Do not show the same entry twice.
        if (!inserted_results.insert(db->detailed_names[i]).second)
          continue;

        InsertSymbolIntoResult(db, working_files, db->symbols[i], &out.result);
        if (out.result.size() >= config->maxWorkspaceSearchResults)
          break;
      }
    }

    if (out.result.size() < config->maxWorkspaceSearchResults) {
      for (int i = 0; i < db->detailed_names.size(); ++i) {
        if (SubstringMatch(query, db->detailed_names[i])) {
          // Do not show the same entry twice.
          if (!inserted_results.insert(db->detailed_names[i]).second)
            continue;

          InsertSymbolIntoResult(db, working_files, db->symbols[i],
                                 &out.result);
          if (out.result.size() >= config->maxWorkspaceSearchResults)
            break;
        }
      }
    }

    LOG_S(INFO) << "[querydb] Found " << out.result.size()
                << " results for query " << query;
    IpcManager::WriteStdout(IpcId::WorkspaceSymbol, out);
  }
};
REGISTER_MESSAGE_HANDLER(WorkspaceSymbolHandler);
}  // namespace