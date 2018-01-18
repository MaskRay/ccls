#pragma once

#include <mutex>
#include <string>
#include <unordered_set>

// Manages files inside of the indexing pipeline so we don't have the same file
// being imported multiple times.
//
// NOTE: This is not thread safe and should only be used on the querydb thread.
struct ImportManager {
  // Try to mark the given dependency as imported. A dependency can only ever be
  // imported once.
  bool TryMarkDependencyImported(const std::string& path);

  // Try to import the given file into querydb. We should only ever be
  // importing a file into querydb once per file. Returns true if the file
  // can be imported.
  bool StartQueryDbImport(const std::string& path);

  // The file has been fully imported and can be imported again later on.
  void DoneQueryDbImport(const std::string& path);

  std::unordered_set<std::string> querydb_processing_;

  // TODO: use std::shared_mutex so we can have multiple readers.
  std::mutex depdency_mutex_;
  std::unordered_set<std::string> depdency_imported_;
};