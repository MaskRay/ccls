#include "cache.h"

#include "indexer.h"

#include <algorithm>

std::string GetCachedFileName(std::string source_file) {
  // TODO/FIXME
  const char* kCacheDirectory = "C:/Users/jacob/Desktop/superindex/indexer/CACHE/";
  std::replace(source_file.begin(), source_file.end(), '\\', '_');
  std::replace(source_file.begin(), source_file.end(), '/', '_');
  std::replace(source_file.begin(), source_file.end(), ':', '_');
  std::replace(source_file.begin(), source_file.end(), '.', '_');
  return kCacheDirectory + source_file + ".json";
}

std::unique_ptr<IndexedFile> LoadCachedFile(std::string filename) {
  return nullptr;

  optional<std::string> file_content = ReadContent(GetCachedFileName(filename));
  if (!file_content)
    return nullptr;

  optional<IndexedFile> indexed = Deserialize(filename, *file_content);
  if (indexed)
    return MakeUnique<IndexedFile>(indexed.value());

  return nullptr;
}

void WriteToCache(std::string filename, IndexedFile& file) {
  std::string indexed_content = Serialize(file);

  std::ofstream cache;
  cache.open(GetCachedFileName(filename));
  assert(cache.good());
  cache << indexed_content;
  cache.close();
}