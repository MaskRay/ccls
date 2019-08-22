// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "include_complete.hh"

#include "filesystem.hh"
#include "platform.hh"
#include "project.hh"

#include <llvm/ADT/Twine.h>
#include <llvm/Support/Threading.h>
#include <llvm/Support/Timer.h>

#include <unordered_set>
using namespace llvm;

#include <thread>

namespace ccls {
namespace {

struct CompletionCandidate {
  std::string absolute_path;
  CompletionItem completion_item;
};

std::string elideLongPath(const std::string &path) {
  if (g_config->completion.include.maxPathSize <= 0 ||
      (int)path.size() <= g_config->completion.include.maxPathSize)
    return path;

  size_t start = path.size() - g_config->completion.include.maxPathSize;
  return ".." + path.substr(start + 2);
}

size_t trimCommonPathPrefix(const std::string &result,
                            const std::string &trimmer) {
#ifdef _WIN32
  std::string s = result, t = trimmer;
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);
  std::transform(t.begin(), t.end(), t.begin(), ::tolower);
  if (s.compare(0, t.size(), t) == 0)
    return t.size();
#else
  if (result.compare(0, trimmer.size(), trimmer) == 0)
    return trimmer.size();
#endif
  return 0;
}

int trimPath(Project *project, std::string &path) {
  size_t pos = 0;
  int kind = 0;
  for (auto &[root, folder] : project->root2folder)
    for (auto &[search, search_dir_kind] : folder.search_dir2kind)
      if (int t = trimCommonPathPrefix(path, search); t > pos)
        pos = t, kind = search_dir_kind;
  path = path.substr(pos);
  return kind;
}

CompletionItem buildCompletionItem(const std::string &path, int kind) {
  CompletionItem item;
  item.label = elideLongPath(path);
  item.detail = path; // the include path, used in de-duplicating
  item.textEdit.newText = path;
  item.insertTextFormat = InsertTextFormat::PlainText;
  item.kind = CompletionItemKind::File;
  item.quote_kind_ = kind;
  item.priority_ = 0;
  return item;
}
} // namespace

IncludeComplete::IncludeComplete(Project *project)
    : is_scanning(false), project_(project) {}

IncludeComplete::~IncludeComplete() {
  // Spin until the scanning has completed.
  while (is_scanning.load())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void IncludeComplete::rescan() {
  if (is_scanning || LLVM_VERSION_MAJOR >= 8)
    return;

  completion_items.clear();
  absolute_path_to_completion_item.clear();
  inserted_paths.clear();

  if (!match_ && (g_config->completion.include.whitelist.size() ||
                  g_config->completion.include.blacklist.size()))
    match_ =
        std::make_unique<GroupMatch>(g_config->completion.include.whitelist,
                                     g_config->completion.include.blacklist);

  is_scanning = true;
  std::thread([this]() {
    set_thread_name("include");
    for (auto &[root, folder] : project_->root2folder) {
      for (auto &search_kind : folder.search_dir2kind) {
        const std::string &search = search_kind.first;
        int kind = search_kind.second;
        assert(search.back() == '/');
        if (match_ && !match_->matches(search))
          return;
        bool include_cpp = search.find("include/c++") != std::string::npos;

        std::vector<CompletionCandidate> results;
        getFilesInFolder(
            search, true /*recursive*/, false /*add_folder_to_path*/,
            [&](const std::string &path) {
              bool ok = include_cpp;
              for (StringRef suffix :
                   g_config->completion.include.suffixWhitelist)
                if (StringRef(path).endswith(suffix))
                  ok = true;
              if (!ok)
                return;
              if (match_ && !match_->matches(search + path))
                return;

              CompletionCandidate candidate;
              candidate.absolute_path = search + path;
              candidate.completion_item = buildCompletionItem(path, kind);
              results.push_back(candidate);
            });

        std::lock_guard lock(completion_items_mutex);
        for (CompletionCandidate &result : results)
          insertCompletionItem(result.absolute_path,
                               std::move(result.completion_item));
      }
    }

    is_scanning = false;
  }).detach();
}

void IncludeComplete::insertCompletionItem(const std::string &absolute_path,
                                           CompletionItem &&item) {
  if (inserted_paths.try_emplace(item.detail, inserted_paths.size()).second) {
    completion_items.push_back(item);
    // insert if not found or with shorter include path
    auto it = absolute_path_to_completion_item.find(absolute_path);
    if (it == absolute_path_to_completion_item.end() ||
        completion_items[it->second].detail.length() > item.detail.length()) {
      absolute_path_to_completion_item[absolute_path] =
          completion_items.size() - 1;
    }
  }
}

void IncludeComplete::addFile(const std::string &path) {
  bool ok = false;
  for (StringRef suffix : g_config->completion.include.suffixWhitelist)
    if (StringRef(path).endswith(suffix))
      ok = true;
  if (!ok)
    return;
  if (match_ && !match_->matches(path))
    return;

  std::string trimmed_path = path;
  int kind = trimPath(project_, trimmed_path);
  CompletionItem item = buildCompletionItem(trimmed_path, kind);

  std::unique_lock<std::mutex> lock(completion_items_mutex, std::defer_lock);
  if (is_scanning)
    lock.lock();
  insertCompletionItem(path, std::move(item));
}

std::optional<CompletionItem>
IncludeComplete::findCompletionItemForAbsolutePath(
    const std::string &absolute_path) {
  std::lock_guard<std::mutex> lock(completion_items_mutex);

  auto it = absolute_path_to_completion_item.find(absolute_path);
  if (it == absolute_path_to_completion_item.end())
    return std::nullopt;
  return completion_items[it->second];
}
} // namespace ccls
