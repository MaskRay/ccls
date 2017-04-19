#include "cache.h"

#include "indexer.h"

#include <algorithm>

namespace {

std::string GetCachedFileName(const std::string& cache_directory, std::string source_file) {
  assert(!cache_directory.empty());
  std::replace(source_file.begin(), source_file.end(), '\\', '_');
  std::replace(source_file.begin(), source_file.end(), '/', '_');
  std::replace(source_file.begin(), source_file.end(), ':', '_');
  std::replace(source_file.begin(), source_file.end(), '.', '_');
  return cache_directory + source_file + ".json";
}

}  // namespace

std::unique_ptr<IndexedFile> LoadCachedFile(IndexerConfig* config, const std::string& filename) {
  if (!config->enableCacheRead)
    return nullptr;

  optional<std::string> file_content = ReadContent(GetCachedFileName(config->cacheDirectory, filename));
  if (!file_content)
    return nullptr;

  optional<IndexedFile> indexed = Deserialize(filename, *file_content);
  if (indexed)
    return MakeUnique<IndexedFile>(indexed.value());

  return nullptr;
}

void WriteToCache(IndexerConfig* config, const std::string& filename, IndexedFile& file) {
  if (!config->enableCacheWrite)
    return;

  std::string indexed_content = Serialize(file);

  std::ofstream cache;
  cache.open(GetCachedFileName(config->cacheDirectory, filename));
  assert(cache.good());
  cache << indexed_content;
  cache.close();
}
