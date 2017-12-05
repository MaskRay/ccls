#pragma once

#include "config.h"

#include <unordered_map>

// Manages loading caches from file paths for the indexer process.
struct CacheLoader {
  explicit CacheLoader(Config* config);

  IndexFile* TryLoad(const std::string& path);

  // Takes the existing cache or loads the cache at |path|. May return nullptr
  // if the cache does not exist.
  std::unique_ptr<IndexFile> TryTakeOrLoad(const std::string& path);

  // Takes the existing cache or loads the cache at |path|. Asserts the cache
  // exists.
  std::unique_ptr<IndexFile> TakeOrLoad(const std::string& path);

  std::unordered_map<std::string, std::unique_ptr<IndexFile>> caches;
  Config* config_;
};