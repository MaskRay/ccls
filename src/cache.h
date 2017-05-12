#pragma once

#include <optional.h>
#include <memory>
#include <string>

using std::experimental::optional;
using std::experimental::nullopt;

struct IndexerConfig;
struct IndexFile;

std::unique_ptr<IndexFile> LoadCachedIndex(IndexerConfig* config,
                                           const std::string& filename);

optional<std::string> LoadCachedFileContents(IndexerConfig* config,
                                             const std::string& filename);

void WriteToCache(IndexerConfig* config,
                  const std::string& filename,
                  IndexFile& file);