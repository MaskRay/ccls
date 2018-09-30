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

#include "include_complete.h"

#include "filesystem.hh"
#include "match.h"
#include "platform.h"
#include "project.h"

#include <llvm/ADT/Twine.h>
#include <llvm/Support/Threading.h>
#include <llvm/Support/Timer.h>
using namespace llvm;

#include <thread>

namespace {

struct CompletionCandidate {
  std::string absolute_path;
  lsCompletionItem completion_item;
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
bool TrimPath(Project *project, const std::string &project_root,
              std::string *insert_path) {
  size_t start = TrimCommonPathPrefix(*insert_path, project_root);
  bool angle = false;

  for (auto &include_dir : project->quote_include_directories)
    start = std::max(start, TrimCommonPathPrefix(*insert_path, include_dir));

  for (auto &include_dir : project->angle_include_directories) {
    auto len = TrimCommonPathPrefix(*insert_path, include_dir);
    if (len > start) {
      start = len;
      angle = true;
    }
  }

  *insert_path = insert_path->substr(start);
  return angle;
}

lsCompletionItem BuildCompletionItem(const std::string &path,
                                     bool use_angle_brackets, bool is_stl) {
  lsCompletionItem item;
  item.label = ElideLongPath(path);
  item.detail = path; // the include path, used in de-duplicating
  item.textEdit = lsTextEdit();
  item.textEdit->newText = path;
  item.insertTextFormat = lsInsertTextFormat::PlainText;
  item.use_angle_brackets_ = use_angle_brackets;
  if (is_stl) {
    item.kind = lsCompletionItemKind::Module;
    item.priority_ = 2;
  } else {
    item.kind = lsCompletionItemKind::File;
    item.priority_ = 1;
  }
  return item;
}

} // namespace

IncludeComplete::IncludeComplete(Project *project)
    : is_scanning(false), project_(project) {}

void IncludeComplete::Rescan() {
  if (is_scanning)
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
    Timer timer("include", "scan include paths");
    TimeRegion region(timer);

    for (const std::string &dir : project_->quote_include_directories)
      InsertIncludesFromDirectory(dir, false /*use_angle_brackets*/);
    for (const std::string &dir : project_->angle_include_directories)
      InsertIncludesFromDirectory(dir, true /*use_angle_brackets*/);

    is_scanning = false;
  })
      .detach();
}

void IncludeComplete::InsertCompletionItem(const std::string &absolute_path,
                                           lsCompletionItem &&item) {
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
    lsCompletionItem &inserted_item =
        completion_items[inserted_paths[item.detail]];
    // Update |use_angle_brackets_|, prefer quotes.
    if (!item.use_angle_brackets_)
      inserted_item.use_angle_brackets_ = false;
  }
}

void IncludeComplete::AddFile(const std::string &absolute_path) {
  if (!EndsWithAny(absolute_path, g_config->completion.include.suffixWhitelist))
    return;
  if (match_ && !match_->IsMatch(absolute_path))
    return;

  std::string trimmed_path = absolute_path;
  bool use_angle_brackets =
      TrimPath(project_, g_config->projectRoot, &trimmed_path);
  lsCompletionItem item =
      BuildCompletionItem(trimmed_path, use_angle_brackets, false /*is_stl*/);

  std::unique_lock<std::mutex> lock(completion_items_mutex, std::defer_lock);
  if (is_scanning)
    lock.lock();
  InsertCompletionItem(absolute_path, std::move(item));
}

void IncludeComplete::InsertIncludesFromDirectory(std::string directory,
                                                  bool use_angle_brackets) {
  directory = NormalizePath(directory);
  EnsureEndsInSlash(directory);
  if (match_ && !match_->IsMatch(directory))
    return;
  bool include_cpp = directory.find("include/c++") != std::string::npos;

  std::vector<CompletionCandidate> results;
  GetFilesInFolder(
      directory, true /*recursive*/, false /*add_folder_to_path*/,
      [&](const std::string &path) {
        if (!include_cpp &&
            !EndsWithAny(path, g_config->completion.include.suffixWhitelist))
          return;
        if (match_ && !match_->IsMatch(directory + path))
          return;

        CompletionCandidate candidate;
        candidate.absolute_path = directory + path;
        candidate.completion_item =
            BuildCompletionItem(path, use_angle_brackets, false /*is_stl*/);
        results.push_back(candidate);
      });

  std::lock_guard<std::mutex> lock(completion_items_mutex);
  for (CompletionCandidate &result : results)
    InsertCompletionItem(result.absolute_path,
                         std::move(result.completion_item));
}

std::optional<lsCompletionItem>
IncludeComplete::FindCompletionItemForAbsolutePath(
    const std::string &absolute_path) {
  std::lock_guard<std::mutex> lock(completion_items_mutex);

  auto it = absolute_path_to_completion_item.find(absolute_path);
  if (it == absolute_path_to_completion_item.end())
    return std::nullopt;
  return completion_items[it->second];
}
