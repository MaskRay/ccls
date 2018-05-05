#pragma once

#include <optional>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct IndexFile;

struct ICacheManager {
  void WriteToCache(IndexFile& file);

  std::optional<std::string> LoadCachedFileContents(const std::string& path);

  template <typename Fn>
  void IterateLoadedCaches(Fn fn) {
    for (const auto& cache : caches_)
      fn(cache.second.get());
  }

  std::unique_ptr<IndexFile> RawCacheLoad(const std::string& path);

  std::unordered_map<std::string, std::unique_ptr<IndexFile>> caches_;
};
