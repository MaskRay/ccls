#include "cache.h"

#include "indexer.h"
#include "platform.h"
#include "language_server_api.h"

#include <algorithm>

namespace {

std::string GetCachedBaseFileName(const std::string& cache_directory,
                                  std::string source_file) {
  assert(!cache_directory.empty());
  std::replace(source_file.begin(), source_file.end(), '\\', '_');
  std::replace(source_file.begin(), source_file.end(), '/', '_');
  std::replace(source_file.begin(), source_file.end(), ':', '_');

  return cache_directory + source_file;
}

}  // namespace

std::unique_ptr<IndexFile> LoadCachedIndex(Config* config,
                                           const std::string& filename) {
  if (!config->enableCacheRead)
    return nullptr;

  optional<std::string> file_content = ReadContent(
      GetCachedBaseFileName(config->cacheDirectory, filename) + ".json");
  if (!file_content)
    return nullptr;

  optional<IndexFile> indexed = Deserialize(filename, *file_content);
  if (indexed && indexed->version == IndexFile::kCurrentVersion)
    return MakeUnique<IndexFile>(indexed.value());

  return nullptr;
}

optional<std::string> LoadCachedFileContents(Config* config,
                                             const std::string& filename) {
  if (!config->enableCacheRead)
    return nullopt;

  return ReadContent(GetCachedBaseFileName(config->cacheDirectory, filename));
}

void WriteToCache(Config* config,
                  const std::string& filename,
                  IndexFile& file,
                  const optional<std::string>& indexed_file_content) {
  if (!config->enableCacheWrite)
    return;

  std::string cache_basename =
      GetCachedBaseFileName(config->cacheDirectory, filename);

  if (indexed_file_content) {
    std::ofstream cache_content;
    cache_content.open(cache_basename);
    assert(cache_content.good());
    cache_content << *indexed_file_content;
    cache_content.close();
  }
  else {
    CopyFileTo(cache_basename, filename);
  }

  std::string indexed_content = Serialize(file);
  std::ofstream cache;
  cache.open(cache_basename + ".json");
  assert(cache.good());
  cache << indexed_content;
  cache.close();
}
