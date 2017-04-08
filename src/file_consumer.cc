#include "file_consumer.h"

#include "indexer.h"
#include "utils.h"

FileConsumer::FileConsumer(SharedState* shared_state) : shared_(shared_state) {}

std::vector<std::unique_ptr<IndexedFile>> FileConsumer::TakeLocalState() {
  std::vector<std::unique_ptr<IndexedFile>> result;
  for (auto& entry : local_) {
    if (entry.second)
      result.push_back(std::move(entry.second));
  }
  return result;
}

IndexedFile* FileConsumer::TryConsumeFile(const std::string& file) {
  // Try to find cached local result.
  auto it = local_.find(file);
  if (it != local_.end())
    return it->second.get();

  // No result in local; we need to query global.
  bool did_insert = false;
  {
    std::lock_guard<std::mutex> lock(shared_->mutex);
    did_insert = shared_->files.insert(file).second;
  }
  local_[file] = did_insert ? MakeUnique<IndexedFile>(file) : nullptr;
  return local_[file].get();
}