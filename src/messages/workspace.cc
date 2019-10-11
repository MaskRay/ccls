// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "fuzzy_match.hh"
#include "log.hh"
#include "message_handler.hh"
#include "pipeline.hh"
#include "project.hh"
#include "query.hh"
#include "sema_manager.hh"

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Path.h>

#include <algorithm>
#include <ctype.h>
#include <functional>
#include <limits.h>
using namespace llvm;

namespace ccls {
REFLECT_STRUCT(SymbolInformation, name, kind, location, containerName);

void MessageHandler::workspace_didChangeConfiguration(EmptyParam &) {
  for (auto &[folder, _] : g_config->workspaceFolders)
    project->load(folder);
  project->index(wfiles, RequestId());

  manager->clear();
};

void MessageHandler::workspace_didChangeWatchedFiles(
    DidChangeWatchedFilesParam &param) {
  for (auto &event : param.changes) {
    std::string path = event.uri.getPath();
    if ((g_config->cache.directory.size() &&
         StringRef(path).startswith(g_config->cache.directory)) ||
        lookupExtension(path).first == LanguageId::Unknown)
      return;
    for (std::string cur = path; cur.size(); cur = sys::path::parent_path(cur))
      if (cur[0] == '.')
        return;

    switch (event.type) {
    case FileChangeType::Created:
    case FileChangeType::Changed: {
      IndexMode mode =
          wfiles->getFile(path) ? IndexMode::Normal : IndexMode::Background;
      pipeline::index(path, {}, mode, true);
      if (event.type == FileChangeType::Changed) {
        if (mode == IndexMode::Normal)
          manager->onSave(path);
        else
          manager->onClose(path);
      }
      break;
    }
    case FileChangeType::Deleted:
      pipeline::index(path, {}, IndexMode::Delete, false);
      manager->onClose(path);
      break;
    }
  }
}

void MessageHandler::workspace_didChangeWorkspaceFolders(
    DidChangeWorkspaceFoldersParam &param) {
  for (const WorkspaceFolder &wf : param.event.removed) {
    std::string root = wf.uri.getPath();
    ensureEndsInSlash(root);
    LOG_S(INFO) << "delete workspace folder " << wf.name << ": " << root;
    auto it = llvm::find_if(g_config->workspaceFolders,
                            [&](auto &folder) { return folder.first == root; });
    if (it != g_config->workspaceFolders.end()) {
      g_config->workspaceFolders.erase(it);
      {
        // auto &folder = project->root2folder[path];
        // FIXME delete
      }
      project->root2folder.erase(root);
    }
  }
  auto &workspaceFolders = g_config->workspaceFolders;
  for (const WorkspaceFolder &wf : param.event.added) {
    std::string folder = wf.uri.getPath();
    ensureEndsInSlash(folder);
    std::string real = realPath(folder) + '/';
    if (folder == real)
      real.clear();
    LOG_S(INFO) << "add workspace folder " << wf.name << ": "
                << (real.empty() ? folder : (folder + " -> ").append(real));
    workspaceFolders.emplace_back();
    auto it = workspaceFolders.end() - 1;
    for (; it != workspaceFolders.begin() && folder < it[-1].first; --it)
      *it = it[-1];
    *it = {folder, real};
    project->load(folder);
  }

  project->index(wfiles, RequestId());

  manager->clear();
}

namespace {
// Lookup |symbol| in |db| and insert the value into |result|.
bool addSymbol(
    DB *db, WorkingFiles *wfiles, const std::vector<uint8_t> &file_set,
    SymbolIdx sym, bool use_detailed,
    std::vector<std::tuple<SymbolInformation, int, SymbolIdx>> *result) {
  std::optional<SymbolInformation> info = getSymbolInfo(db, sym, true);
  if (!info)
    return false;

  Maybe<DeclRef> dr;
  bool in_folder = false;
  withEntity(db, sym, [&](const auto &entity) {
    for (auto &def : entity.def)
      if (def.spell) {
        dr = def.spell;
        if (!in_folder && (in_folder = file_set[def.spell->file_id]))
          break;
      }
  });
  if (!dr) {
    auto &decls = getNonDefDeclarations(db, sym);
    for (auto &dr1 : decls) {
      dr = dr1;
      if (!in_folder && (in_folder = file_set[dr1.file_id]))
        break;
    }
  }
  if (!in_folder)
    return false;

  std::optional<Location> ls_location = getLsLocation(db, wfiles, *dr);
  if (!ls_location)
    return false;
  info->location = *ls_location;
  result->emplace_back(*info, int(use_detailed), sym);
  return true;
}
} // namespace

void MessageHandler::workspace_symbol(WorkspaceSymbolParam &param,
                                      ReplyOnce &reply) {
  std::vector<SymbolInformation> result;
  const std::string &query = param.query;
  for (auto &folder : param.folders)
    ensureEndsInSlash(folder);
  std::vector<uint8_t> file_set = db->getFileSet(param.folders);

  // {symbol info, matching detailed_name or short_name, index}
  std::vector<std::tuple<SymbolInformation, int, SymbolIdx>> cands;
  bool sensitive = g_config->workspaceSymbol.caseSensitivity;

  // Find subsequence matches.
  std::string query_without_space;
  query_without_space.reserve(query.size());
  for (char c : query)
    if (!isspace(c))
      query_without_space += c;

  auto add = [&](SymbolIdx sym) {
    std::string_view detailed_name = db->getSymbolName(sym, true);
    int pos = reverseSubseqMatch(query_without_space, detailed_name, sensitive);
    return pos >= 0 &&
           addSymbol(db, wfiles, file_set, sym,
                     detailed_name.find(':', pos) != std::string::npos,
                     &cands) &&
           cands.size() >= g_config->workspaceSymbol.maxNum;
  };
  for (auto &func : db->funcs)
    if (add({func.usr, Kind::Func}))
      goto done_add;
  for (auto &type : db->types)
    if (add({type.usr, Kind::Type}))
      goto done_add;
  for (auto &var : db->vars)
    if (var.def.size() && !var.def[0].is_local() && add({var.usr, Kind::Var}))
      goto done_add;
done_add:

  if (g_config->workspaceSymbol.sort && query.size() <= FuzzyMatcher::kMaxPat) {
    // Sort results with a fuzzy matching algorithm.
    int longest = 0;
    for (auto &cand : cands)
      longest = std::max(
          longest, int(db->getSymbolName(std::get<2>(cand), true).size()));
    FuzzyMatcher fuzzy(query, g_config->workspaceSymbol.caseSensitivity);
    for (auto &cand : cands)
      std::get<1>(cand) = fuzzy.match(
          db->getSymbolName(std::get<2>(cand), std::get<1>(cand)), false);
    std::sort(cands.begin(), cands.end(), [](const auto &l, const auto &r) {
      return std::get<1>(l) > std::get<1>(r);
    });
    result.reserve(cands.size());
    for (auto &cand : cands) {
      // Discard awful candidates.
      if (std::get<1>(cand) <= FuzzyMatcher::kMinScore)
        break;
      result.push_back(std::get<0>(cand));
    }
  } else {
    result.reserve(cands.size());
    for (auto &cand : cands)
      result.push_back(std::get<0>(cand));
  }

  reply(result);
}
} // namespace ccls
