#pragma once

#include "language_server_api.h"

#include <memory>
#include <string>

struct IndexedFile;

std::unique_ptr<IndexedFile> LoadCachedFile(IndexerConfig* config, const std::string& filename);

void WriteToCache(IndexerConfig* config, const std::string& filename, IndexedFile& file);