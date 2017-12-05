#include "cache_loader.h"

#include "cache.h"
#include "indexer.h"

CacheLoader::CacheLoader(Config* config) : config_(config) {}

IndexFile* CacheLoader::TryLoad(const std::string& path) {
  auto it = caches.find(path);
  if (it != caches.end())
    return it->second.get();

  std::unique_ptr<IndexFile> cache = LoadCachedIndex(config_, path);
  if (!cache)
    return nullptr;

  caches[path] = std::move(cache);
  return caches[path].get();
}

std::unique_ptr<IndexFile> CacheLoader::TryTakeOrLoad(const std::string& path) {
  auto it = caches.find(path);
  if (it != caches.end()) {
    auto result = std::move(it->second);
    caches.erase(it);
    return result;
  }

  return LoadCachedIndex(config_, path);
}

std::unique_ptr<IndexFile> CacheLoader::TakeOrLoad(const std::string& path) {
  auto result = TryTakeOrLoad(path);
  assert(result);
  return result;
}
