#pragma once

#include <optional.h>

#include <functional>
#include <memory>
#include <string>

struct Config;
struct IndexFile;

struct ICacheManager {
  struct FakeCacheEntry {
    std::string path;
    std::string content;
    std::string json;
  };

  static std::unique_ptr<ICacheManager> Make(Config* config);
  static std::unique_ptr<ICacheManager> MakeFake(
      const std::vector<FakeCacheEntry>& entries);

  virtual ~ICacheManager();

  // Tries to load a cache for |path|, returning null if there is none. The
  // cache loader still owns the cache.
  virtual IndexFile* TryLoad(const std::string& path) = 0;

  // Takes the existing cache or loads the cache at |path|. May return null if
  // the cache does not exist.
  virtual std::unique_ptr<IndexFile> TryTakeOrLoad(const std::string& path) = 0;

  // Takes the existing cache or loads the cache at |path|. Asserts the cache
  // exists.
  std::unique_ptr<IndexFile> TakeOrLoad(const std::string& path);

  virtual optional<std::string> LoadCachedFileContents(
      const std::string& filename) = 0;

  // Iterate over all loaded caches.
  virtual void IterateLoadedCaches(std::function<void(IndexFile*)> fn) = 0;
};

void WriteToCache(Config* config, IndexFile& file);