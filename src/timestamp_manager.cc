#include "timestamp_manager.h"

#include "cache_manager.h"
#include "indexer.h"

optional<int64_t> TimestampManager::GetLastCachedModificationTime(
    ICacheManager* cache_manager,
    const std::string& path) {
  {
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = timestamps_.find(path);
    if (it != timestamps_.end())
      return it->second;
  }
  IndexFile* file = cache_manager->TryLoad(path);
  if (!file)
    return nullopt;

  UpdateCachedModificationTime(path, file->last_modification_time);
  return file->last_modification_time;
}

void TimestampManager::UpdateCachedModificationTime(const std::string& path,
                                                    int64_t timestamp) {
  std::lock_guard<std::mutex> guard(mutex_);
  timestamps_[path] = timestamp;
}
