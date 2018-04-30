#include "cache_manager.h"

#include "config.h"
#include "indexer.h"
#include "lsp.h"
#include "platform.h"

#include <loguru/loguru.hpp>

#include <algorithm>
#include <unordered_map>

namespace {

// Manages loading caches from file paths for the indexer process.
struct RealCacheManager : ICacheManager {
  explicit RealCacheManager() {}
  ~RealCacheManager() override = default;

  void WriteToCache(IndexFile& file) override {
    std::string cache_path = GetCachePath(file.path);
    WriteToFile(cache_path, file.file_contents);

    std::string indexed_content = Serialize(g_config->cacheFormat, file);
    WriteToFile(AppendSerializationFormat(cache_path), indexed_content);
  }

  std::optional<std::string> LoadCachedFileContents(
      const std::string& path) override {
    return ReadContent(GetCachePath(path));
  }

  std::unique_ptr<IndexFile> RawCacheLoad(const std::string& path) override {
    std::string cache_path = GetCachePath(path);
    std::optional<std::string> file_content = ReadContent(cache_path);
    std::optional<std::string> serialized_indexed_content =
        ReadContent(AppendSerializationFormat(cache_path));
    if (!file_content || !serialized_indexed_content)
      return nullptr;

    return Deserialize(g_config->cacheFormat, path, *serialized_indexed_content,
                       *file_content, IndexFile::kMajorVersion);
  }

  std::string GetCachePath(const std::string& source_file) {
    assert(!g_config->cacheDirectory.empty());
    std::string cache_file;
    size_t len = g_config->projectRoot.size();
    if (StartsWith(source_file, g_config->projectRoot)) {
      cache_file = EscapeFileName(g_config->projectRoot) +
                   EscapeFileName(source_file.substr(len));
    } else {
      cache_file = '@' + EscapeFileName(g_config->projectRoot) +
                   EscapeFileName(source_file);
    }

    return g_config->cacheDirectory + cache_file;
  }

  std::string AppendSerializationFormat(const std::string& base) {
    switch (g_config->cacheFormat) {
      case SerializeFormat::Binary:
        return base + ".blob";
      case SerializeFormat::Json:
        return base + ".json";
    }
  }
};

struct FakeCacheManager : ICacheManager {
  explicit FakeCacheManager(const std::vector<FakeCacheEntry>& entries)
      : entries_(entries) {}

  void WriteToCache(IndexFile& file) override { assert(false); }

  std::optional<std::string> LoadCachedFileContents(
      const std::string& path) override {
    for (const FakeCacheEntry& entry : entries_) {
      if (entry.path == path) {
        return entry.content;
      }
    }

    return std::nullopt;
  }

  std::unique_ptr<IndexFile> RawCacheLoad(const std::string& path) override {
    for (const FakeCacheEntry& entry : entries_) {
      if (entry.path == path) {
        return Deserialize(SerializeFormat::Json, path, entry.json, "<empty>",
                           std::nullopt);
      }
    }

    return nullptr;
  }

  std::vector<FakeCacheEntry> entries_;
};

}  // namespace

// static
std::shared_ptr<ICacheManager> ICacheManager::Make() {
  return std::make_shared<RealCacheManager>();
}

// static
std::shared_ptr<ICacheManager> ICacheManager::MakeFake(
    const std::vector<FakeCacheEntry>& entries) {
  return std::make_shared<FakeCacheManager>(entries);
}

ICacheManager::~ICacheManager() = default;

IndexFile* ICacheManager::TryLoad(const std::string& path) {
  auto it = caches_.find(path);
  if (it != caches_.end())
    return it->second.get();

  std::unique_ptr<IndexFile> cache = RawCacheLoad(path);
  if (!cache)
    return nullptr;

  caches_[path] = std::move(cache);
  return caches_[path].get();
}

std::unique_ptr<IndexFile> ICacheManager::TryTakeOrLoad(
    const std::string& path) {
  auto it = caches_.find(path);
  if (it != caches_.end()) {
    auto result = std::move(it->second);
    caches_.erase(it);
    return result;
  }

  return RawCacheLoad(path);
}
