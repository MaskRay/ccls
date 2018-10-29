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

#pragma once

#include "message_handler.hh"

#include <atomic>
#include <mutex>

namespace ccls {
struct GroupMatch;
struct Project;

struct IncludeComplete {
  IncludeComplete(Project *project);

  // Starts scanning directories. Clears existing cache.
  void Rescan();

  // Ensures the one-off file is inside |completion_items|.
  void AddFile(const std::string &absolute_path);

  // Scans the given directory and inserts all includes from this. This is a
  // blocking function and should be run off the querydb thread.
  void InsertIncludesFromDirectory(std::string directory,
                                   bool use_angle_brackets);

  std::optional<ccls::lsCompletionItem>
  FindCompletionItemForAbsolutePath(const std::string &absolute_path);

  // Insert item to |completion_items|.
  // Update |absolute_path_to_completion_item| and |inserted_paths|.
  void InsertCompletionItem(const std::string &absolute_path,
                            ccls::lsCompletionItem &&item);

  // Guards |completion_items| when |is_scanning| is true.
  std::mutex completion_items_mutex;
  std::atomic<bool> is_scanning;
  std::vector<ccls::lsCompletionItem> completion_items;

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
