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
  void InsertIncludesFromDirectory(std::string directory, bool use_angle_brackets);
  void InsertStlIncludes();

  // Guards |completion_items| when |is_scanning| is true.
  std::mutex completion_items_mutex;
  std::atomic<bool> is_scanning;
  std::vector<lsCompletionItem> completion_items;
  // Paths inside of |completion_items|. Multiple paths could show up the same
  // time, but with different bracket types, so we have to hash on the absolute
  // path, and not what we insert into the completion results.
  std::unordered_set<std::string> seen_paths;

  // Cached references
  Config* config_;
  Project* project_;
  std::unique_ptr<GroupMatch> match_;
};

