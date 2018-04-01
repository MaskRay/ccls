#pragma once

#include <optional>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct Config;
struct IndexFile;

struct ICacheManager {
  struct FakeCacheEntry {
    std::string path;
    std::string content;
    std::string json;
  };

  static std::shared_ptr<ICacheManager> Make();
  static std::shared_ptr<ICacheManager> MakeFake(
      const std::vector<FakeCacheEntry>& entries);

  virtual ~ICacheManager();

  // Tries to load a cache for |path|, returning null if there is none. The
  // cache loader still owns the cache.
  IndexFile* TryLoad(const std::string& path);

  // Takes the existing cache or loads the cache at |path|. May return null if
  // the cache does not exist.
  std::unique_ptr<IndexFile> TryTakeOrLoad(const std::string& path);

  // Takes the existing cache or loads the cache at |path|. Asserts the cache
  // exists.
  std::unique_ptr<IndexFile> TakeOrLoad(const std::string& path);

  virtual void WriteToCache(IndexFile& file) = 0;

  virtual std::optional<std::string> LoadCachedFileContents(
      const std::string& path) = 0;

  // Iterate over all loaded caches.
  void IterateLoadedCaches(std::function<void(IndexFile*)> fn);

 protected:
  virtual std::unique_ptr<IndexFile> RawCacheLoad(const std::string& path) = 0;
  std::unordered_map<std::string, std::unique_ptr<IndexFile>> caches_;
};
