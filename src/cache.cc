#include "cache.h"

#include "indexer.h"
#include "language_server_api.h"
#include "platform.h"

#include <loguru/loguru.hpp>

#include <algorithm>

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

}  // namespace

std::unique_ptr<IndexFile> LoadCachedIndex(Config* config,
                                           const std::string& filename) {
  if (!config->enableCacheRead)
    return nullptr;

  optional<std::string> file_content = ReadContent(
      GetCachedBaseFileName(config, filename) + ".json");
  if (!file_content)
    return nullptr;

  return Deserialize(filename, *file_content, IndexFile::kCurrentVersion);
}

optional<std::string> LoadCachedFileContents(Config* config,
                                             const std::string& filename) {
  if (!config->enableCacheRead)
    return nullopt;

  return ReadContent(GetCachedBaseFileName(config, filename));
}

void WriteToCache(Config* config, IndexFile& file) {
  if (!config->enableCacheWrite)
    return;

  std::string cache_basename =
      GetCachedBaseFileName(config, file.path);

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
