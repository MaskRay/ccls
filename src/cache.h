#pragma once

#include <memory>
#include <string>

struct IndexedFile;

std::unique_ptr<IndexedFile> LoadCachedFile(const std::string& cache_directory, const std::string& filename);

void WriteToCache(const std::string& cache_directory, const std::string& filename, IndexedFile& file);