#pragma once

#include "language_server_api.h"

#include <atomic>
#include <mutex>
#include <unordered_set>

struct GroupMatch;
struct Project;

struct IncludeComplete {
  IncludeComplete(Config* config, Project* project);

  // Starts scanning directories. Clears existing cache.
  void Rescan();

  // Ensures the one-off file is inside |completion_items|.
  void AddFile(const std::string& absolute_path);

  // Scans the given directory and inserts all includes from this. This is a
  // blocking function and should be run off the querydb thread.
  void InsertIncludesFromDirectory(std::string directory,
                                   bool use_angle_brackets);
  void InsertStlIncludes();

  optional<lsCompletionItem> FindCompletionItemForAbsolutePath(
      const std::string& absolute_path);

  // Guards |completion_items| when |is_scanning| is true.
  std::mutex completion_items_mutex;
  std::atomic<bool> is_scanning;
  std::vector<lsCompletionItem> completion_items;

  // Absolute file path to the completion item in |completion_items|. Also
  // verifies that we only have one completion item per absolute path.
  // We cannot just scan |completion_items| for this information because the
  // same path can often be epxressed in mutliple ways; a trivial example is
  // angle vs quote include style (ie, <foo> vs "foo").
  std::unordered_map<std::string, int> absolute_path_to_completion_item;

  // Cached references
  Config* config_;
  Project* project_;
  std::unique_ptr<GroupMatch> match_;
};
