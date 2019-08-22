// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "message_handler.hh"

#include <atomic>
#include <mutex>

namespace ccls {
struct GroupMatch;
struct Project;

struct IncludeComplete {
  IncludeComplete(Project *project);
  ~IncludeComplete();

  // Starts scanning directories. Clears existing cache.
  void rescan();

  // Ensures the one-off file is inside |completion_items|.
  void addFile(const std::string &absolute_path);

  std::optional<ccls::CompletionItem>
  findCompletionItemForAbsolutePath(const std::string &absolute_path);

  // Insert item to |completion_items|.
  // Update |absolute_path_to_completion_item| and |inserted_paths|.
  void insertCompletionItem(const std::string &absolute_path,
                            ccls::CompletionItem &&item);

  // Guards |completion_items| when |is_scanning| is true.
  std::mutex completion_items_mutex;
  std::atomic<bool> is_scanning;
  std::vector<ccls::CompletionItem> completion_items;

  // Absolute file path to the completion item in |completion_items|.
  // Keep the one with shortest include path.
  std::unordered_map<std::string, int> absolute_path_to_completion_item;

  // Only one completion item per include path.
  std::unordered_map<std::string, int> inserted_paths;

  // Cached references
  Project *project_;
  std::unique_ptr<GroupMatch> match_;
};
} // namespace ccls
