#include "cache.h"

#include "indexer.h"
#include "platform.h"
#include "language_server_api.h"

#include <algorithm>

namespace {

std::string GetCachedBaseFileName(const std::string& cache_directory, std::string source_file) {
  assert(!cache_directory.empty());
  std::replace(source_file.begin(), source_file.end(), '\\', '_');
  std::replace(source_file.begin(), source_file.end(), '/', '_');
  std::replace(source_file.begin(), source_file.end(), ':', '_');
  std::replace(source_file.begin(), source_file.end(), '.', '_');
  return cache_directory + source_file;
}

}  // namespace

std::unique_ptr<IndexedFile> LoadCachedIndex(IndexerConfig* config, const std::string& filename) {
  if (!config->enableCacheRead)
    return nullptr;

  optional<std::string> file_content = ReadContent(GetCachedBaseFileName(config->cacheDirectory, filename) + ".json");
  if (!file_content)
    return nullptr;

  optional<IndexedFile> indexed = Deserialize(filename, *file_content);
  if (indexed && indexed->version == IndexedFile::kCurrentVersion)
    return MakeUnique<IndexedFile>(indexed.value());

  return nullptr;
}

optional<std::string> LoadCachedFileContents(IndexerConfig* config, const std::string& filename) {
  if (!config->enableCacheRead)
    return nullopt;

  return ReadContent(GetCachedBaseFileName(config->cacheDirectory, filename) + ".txt");
}

void WriteToCache(IndexerConfig* config, const std::string& filename, IndexedFile& file) {
  if (!config->enableCacheWrite)
    return;

  std::string cache_basename = GetCachedBaseFileName(config->cacheDirectory, filename);

  CopyFileTo(cache_basename + ".txt", filename);

  std::string indexed_content = Serialize(file);
  std::ofstream cache;
  cache.open(cache_basename + ".json");
  assert(cache.good());
  cache << indexed_content;
  cache.close();
}
