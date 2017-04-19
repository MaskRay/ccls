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

std::unique_ptr<IndexedFile> LoadCachedFile(const std::string& cache_directory, const std::string& filename) {
  return nullptr;

  optional<std::string> file_content = ReadContent(GetCachedFileName(cache_directory, filename));
  if (!file_content)
    return nullptr;

  optional<IndexedFile> indexed = Deserialize(filename, *file_content);
  if (indexed)
    return MakeUnique<IndexedFile>(indexed.value());

  return nullptr;
}

void WriteToCache(const std::string& cache_directory, const std::string& filename, IndexedFile& file) {
  std::string indexed_content = Serialize(file);

  std::ofstream cache;
  cache.open(GetCachedFileName(cache_directory, filename));
  assert(cache.good());
  cache << indexed_content;
  cache.close();
}
