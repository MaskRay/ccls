#pragma once

#include "lru_cache.h"
#include "query.h"

#include <string>
#include <unordered_map>

// Caches symbols for a single file for semantic highlighting to provide
// relatively stable ids. Only supports xxx files at a time.
struct SemanticHighlightSymbolCache {
  struct Entry {
    // The path this cache belongs to.
    std::string path;
    // Detailed symbol name to stable id.
    using TNameToId = std::unordered_map<std::string, int>;
    TNameToId detailed_type_name_to_stable_id;
    TNameToId detailed_func_name_to_stable_id;
    TNameToId detailed_var_name_to_stable_id;

    explicit Entry(const std::string& path);

    int GetStableId(SymbolKind kind, const std::string& detailed_name);
  };

  constexpr static int kCacheSize = 10;
  LruCache<std::string, Entry> cache_;

  SemanticHighlightSymbolCache();

  std::shared_ptr<Entry> GetCacheForFile(const std::string& path);
};
