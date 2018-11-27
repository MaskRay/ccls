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

std::string ElideLongPath(const std::string &path) {
  if (g_config->completion.include.maxPathSize <= 0 ||
      (int)path.size() <= g_config->completion.include.maxPathSize)
    return path;

  size_t start = path.size() - g_config->completion.include.maxPathSize;
  return ".." + path.substr(start + 2);
}

size_t TrimCommonPathPrefix(const std::string &result,
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

// Returns true iff angle brackets should be used.
bool TrimPath(Project *project, std::string &path) {
  size_t pos = 0;
  bool angle = false;
  for (auto &[root, folder] : project->root2folder) {
    size_t pos1 = 0;
    for (auto &search : folder.angle_search_list)
      pos1 = std::max(pos1, TrimCommonPathPrefix(path, search));
    if (pos1 > pos) {
      pos = pos1;
      angle = true;
    }

    pos1 = TrimCommonPathPrefix(path, root);
    for (auto &search : folder.quote_search_list)
      pos1 = std::max(pos1, TrimCommonPathPrefix(path, search));
    if (pos1 > pos) {
      pos = pos1;
      angle = false;
    }
  }
  path = path.substr(pos);
  return angle;
}

CompletionItem BuildCompletionItem(const std::string &path,
                                   bool use_angle_brackets) {
  CompletionItem item;
  item.label = ElideLongPath(path);
  item.detail = path; // the include path, used in de-duplicating
  item.textEdit.newText = path;
  item.insertTextFormat = InsertTextFormat::PlainText;
  item.use_angle_brackets_ = use_angle_brackets;
  item.kind = CompletionItemKind::File;
  item.priority_ = 0;
  return item;
}
} // namespace

IncludeComplete::IncludeComplete(Project *project)
    : is_scanning(false), project_(project) {}

void IncludeComplete::Rescan() {
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
    std::unordered_set<std::string> angle_set, quote_set;
    for (auto &[root, folder] : project_->root2folder) {
      for (const std::string &search : folder.angle_search_list)
        if (angle_set.insert(search).second)
          InsertIncludesFromDirectory(search, true);
      for (const std::string &search : folder.quote_search_list)
        if (quote_set.insert(search).second)
          InsertIncludesFromDirectory(search, false);
    }

    is_scanning = false;
  })
      .detach();
}

void IncludeComplete::InsertCompletionItem(const std::string &absolute_path,
                                           CompletionItem &&item) {
  if (inserted_paths.insert({item.detail, inserted_paths.size()}).second) {
    completion_items.push_back(item);
    // insert if not found or with shorter include path
    auto it = absolute_path_to_completion_item.find(absolute_path);
    if (it == absolute_path_to_completion_item.end() ||
        completion_items[it->second].detail.length() > item.detail.length()) {
      absolute_path_to_completion_item[absolute_path] =
          completion_items.size() - 1;
    }
  } else {
    CompletionItem &inserted_item =
        completion_items[inserted_paths[item.detail]];
    // Update |use_angle_brackets_|, prefer quotes.
    if (!item.use_angle_brackets_)
      inserted_item.use_angle_brackets_ = false;
  }
}

void IncludeComplete::AddFile(const std::string &path) {
  bool ok = false;
  for (StringRef suffix : g_config->completion.include.suffixWhitelist)
    if (StringRef(path).endswith(suffix))
      ok = true;
  if (!ok)
    return;
  if (match_ && !match_->Matches(path))
    return;

  std::string trimmed_path = path;
  bool use_angle_brackets = TrimPath(project_, trimmed_path);
  CompletionItem item = BuildCompletionItem(trimmed_path, use_angle_brackets);

  std::unique_lock<std::mutex> lock(completion_items_mutex, std::defer_lock);
  if (is_scanning)
    lock.lock();
  InsertCompletionItem(path, std::move(item));
}

void IncludeComplete::InsertIncludesFromDirectory(std::string directory,
                                                  bool use_angle_brackets) {
  directory = NormalizePath(directory);
  EnsureEndsInSlash(directory);
  if (match_ && !match_->Matches(directory))
    return;
  bool include_cpp = directory.find("include/c++") != std::string::npos;

  std::vector<CompletionCandidate> results;
  GetFilesInFolder(
      directory, true /*recursive*/, false /*add_folder_to_path*/,
      [&](const std::string &path) {
        bool ok = include_cpp;
        for (StringRef suffix : g_config->completion.include.suffixWhitelist)
          if (StringRef(path).endswith(suffix))
            ok = true;
        if (!ok)
          return;
        if (match_ && !match_->Matches(directory + path))
          return;

        CompletionCandidate candidate;
        candidate.absolute_path = directory + path;
        candidate.completion_item =
            BuildCompletionItem(path, use_angle_brackets);
        results.push_back(candidate);
      });

  std::lock_guard<std::mutex> lock(completion_items_mutex);
  for (CompletionCandidate &result : results)
    InsertCompletionItem(result.absolute_path,
                         std::move(result.completion_item));
}

std::optional<CompletionItem>
IncludeComplete::FindCompletionItemForAbsolutePath(
    const std::string &absolute_path) {
  std::lock_guard<std::mutex> lock(completion_items_mutex);

  auto it = absolute_path_to_completion_item.find(absolute_path);
  if (it == absolute_path_to_completion_item.end())
    return std::nullopt;
  return completion_items[it->second];
}
} // namespace ccls
