#pragma once

#include "language_server_api.h"

#include <atomic>
#include <mutex>
#include <unordered_set>

struct GroupMatch;
struct Project;

struct IncludeCompletion {
  IncludeCompletion(Config* config, Project* project);

  // Starts scanning directories. Clears existing cache.
  void Rescan();

  // Ensures the one-off file is inside |completion_items|.
  void AddFile(std::string path);

  // Scans the given directory and inserts all includes from this. This is a 
  // blocking function and should be run off the querydb thread.
  void InsertIncludesFromDirectory(const std::string& directory, bool use_angle_brackets);
  void InsertStlIncludes();

  // Guards |completion_items| when |is_scanning| is true.
  std::mutex completion_items_mutex;
  std::atomic<bool> is_scanning;
  std::unordered_set<lsCompletionItem> completion_items;

  // Cached references
  Config* config_;
  Project* project_;
  std::unique_ptr<GroupMatch> match_;
};

