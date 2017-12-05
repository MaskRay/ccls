#include "code_complete_cache.h"

void CodeCompleteCache::WithLock(std::function<void()> action) {
  std::lock_guard<std::mutex> lock(mutex_);
  action();
}

bool CodeCompleteCache::IsCacheValid(lsTextDocumentPositionParams position) {
  std::lock_guard<std::mutex> lock(mutex_);
  return cached_path_ == position.textDocument.uri.GetPath() &&
         cached_completion_position_ == position.position;
}