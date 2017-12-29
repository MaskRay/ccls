#pragma once

#include <optional.h>

#include <memory>
#include <string>

struct Config;
struct IndexFile;

std::unique_ptr<IndexFile> LoadCachedIndex(Config* config,
                                           const std::string& filename);

optional<std::string> LoadCachedFileContents(Config* config,
                                             const std::string& filename);

void WriteToCache(Config* config, IndexFile& file);