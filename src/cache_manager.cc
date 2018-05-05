#include "cache_manager.h"

#include "config.h"
#include "indexer.h"
#include "lsp.h"
#include "platform.h"

#include <loguru/loguru.hpp>

#include <algorithm>
#include <unordered_map>

namespace {
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
}

// Manages loading caches from file paths for the indexer process.
void ICacheManager::WriteToCache(IndexFile& file) {
  std::string cache_path = GetCachePath(file.path);
  WriteToFile(cache_path, file.file_contents);

  std::string indexed_content = Serialize(g_config->cacheFormat, file);
  WriteToFile(AppendSerializationFormat(cache_path), indexed_content);
}

std::optional<std::string> ICacheManager::LoadCachedFileContents(
    const std::string& path) {
  return ReadContent(GetCachePath(path));
}

std::unique_ptr<IndexFile> ICacheManager::RawCacheLoad(
    const std::string& path) {
  std::string cache_path = GetCachePath(path);
  std::optional<std::string> file_content = ReadContent(cache_path);
  std::optional<std::string> serialized_indexed_content =
      ReadContent(AppendSerializationFormat(cache_path));
  if (!file_content || !serialized_indexed_content)
    return nullptr;

  return Deserialize(g_config->cacheFormat, path, *serialized_indexed_content,
                     *file_content, IndexFile::kMajorVersion);
}
