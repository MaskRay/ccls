#include "semantic_highlight_symbol_cache.h"

SemanticHighlightSymbolCache::Entry::Entry(const std::string& path)
    : path(path) {}

int SemanticHighlightSymbolCache::Entry::GetStableId(
    SymbolKind kind,
    const std::string& detailed_name) {
  TNameToId* map = nullptr;
  switch (kind) {
    case SymbolKind::Type:
      map = &detailed_type_name_to_stable_id;
      break;
    case SymbolKind::Func:
      map = &detailed_func_name_to_stable_id;
      break;
    case SymbolKind::Var:
      map = &detailed_var_name_to_stable_id;
      break;
    default:
      assert(false);
      return 0;
  }
  assert(map);
  auto it = map->find(detailed_name);
  if (it != map->end())
    return it->second;
  return (*map)[detailed_name] = map->size();
}

SemanticHighlightSymbolCache::SemanticHighlightSymbolCache()
    : cache_(kCacheSize) {}

std::shared_ptr<SemanticHighlightSymbolCache::Entry>
SemanticHighlightSymbolCache::GetCacheForFile(const std::string& path) {
  return cache_.Get(path, [&]() { return std::make_shared<Entry>(path); });
}
