#pragma once

#include <mutex>
#include <string>
#include <unordered_set>

// Manages files inside of the indexing pipeline so we don't have the same file
// being imported multiple times.
//
// NOTE: This is not thread safe and should only be used on the querydb thread.
struct ImportManager {
  std::unordered_set<std::string> querydb_processing_;

  // TODO: use std::shared_mutex so we can have multiple readers.
  std::mutex dependency_mutex_;
  std::unordered_set<std::string> dependency_imported_;
};
