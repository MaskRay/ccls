#include "cache_manager.h"

#include "config.h"
#include "indexer.h"
#include "language_server_api.h"
#include "platform.h"

#include <loguru/loguru.hpp>

#include <algorithm>
#include <unordered_map>

namespace {

std::string GetCachedBaseFileName(Config* config,
                                  const std::string& source_file,
                                  bool create_dir = false) {
  assert(!config->cacheDirectory.empty());
  std::string cache_file;
  size_t len = config->projectRoot.size();
  if (StartsWith(source_file, config->projectRoot)) {
    cache_file = EscapeFileName(config->projectRoot) + '/' +
                 EscapeFileName(source_file.substr(len));
  } else
    cache_file = EscapeFileName(source_file);

  return config->cacheDirectory + cache_file;
}

std::unique_ptr<IndexFile> LoadCachedIndex(Config* config,
                                           const std::string& filename) {
  if (!config->enableCacheRead)
    return nullptr;

  optional<std::string> file_content =
      ReadContent(GetCachedBaseFileName(config, filename) + ".json");
  if (!file_content)
    return nullptr;

  return Deserialize(filename, *file_content, IndexFile::kCurrentVersion);
}

// Manages loading caches from file paths for the indexer process.
struct RealCacheManager : ICacheManager {
  explicit RealCacheManager(Config* config) : config_(config) {}
  ~RealCacheManager() override = default;

  IndexFile* TryLoad(const std::string& path) override {
    auto it = caches.find(path);
    if (it != caches.end())
      return it->second.get();

    std::unique_ptr<IndexFile> cache = LoadCachedIndex(config_, path);
    if (!cache)
      return nullptr;

    caches[path] = std::move(cache);
    return caches[path].get();
  }

  std::unique_ptr<IndexFile> TryTakeOrLoad(const std::string& path) override {
    auto it = caches.find(path);
    if (it != caches.end()) {
      auto result = std::move(it->second);
      caches.erase(it);
      return result;
    }

    return LoadCachedIndex(config_, path);
  }

  optional<std::string> LoadCachedFileContents(
      const std::string& filename) override {
    if (!config_->enableCacheRead)
      return nullopt;

    return ReadContent(GetCachedBaseFileName(config_, filename));
  }

  void IterateLoadedCaches(std::function<void(IndexFile*)> fn) override {
    for (const auto& it : caches) {
      assert(it.second);
      fn(it.second.get());
    }
  }

  std::unordered_map<std::string, std::unique_ptr<IndexFile>> caches;
  Config* config_;
};

// struct FakeCacheManager : ICacheManager {
//   explicit FakeCacheManager(const std::vector<FakeCacheEntry>& entries) {
//     assert(false && "TODO");
//   }
// };

}  // namespace

// static
std::unique_ptr<ICacheManager> ICacheManager::Make(Config* config) {
  return MakeUnique<RealCacheManager>(config);
}

// static
std::unique_ptr<ICacheManager> ICacheManager::MakeFake(
    const std::vector<FakeCacheEntry>& entries) {
  // return MakeUnique<FakeCacheManager>(entries);
  assert(false && "TODO");
  return nullptr;
}

ICacheManager::~ICacheManager() = default;

std::unique_ptr<IndexFile> ICacheManager::TakeOrLoad(const std::string& path) {
  auto result = TryTakeOrLoad(path);
  assert(result);
  return result;
}

void WriteToCache(Config* config, IndexFile& file) {
  if (!config->enableCacheWrite)
    return;

  std::string cache_basename = GetCachedBaseFileName(config, file.path);

  if (file.file_contents_.empty()) {
    LOG_S(ERROR) << "No cached file contents; performing potentially stale "
                 << "file-copy for " << file.path;
    CopyFileTo(cache_basename, file.path);
  } else {
    std::ofstream cache_content;
    cache_content.open(cache_basename);
    assert(cache_content.good());
    cache_content << file.file_contents_;
    cache_content.close();
  }

  std::string indexed_content = Serialize(file);
  std::ofstream cache;
  cache.open(cache_basename + ".json");
  assert(cache.good());
  cache << indexed_content;
  cache.close();
}
