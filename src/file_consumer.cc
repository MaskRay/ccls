#include "file_consumer.h"

FileConsumer::FileConsumer(SharedState* shared_state) : shared_(shared_state) {}

void FileConsumer::ClearOwnership() {
  for (auto& entry : local_)
    entry.second = Ownership::DoesNotOwn;
}

bool FileConsumer::DoesOwnFile(const std::string& file) {
  // Try to find cached local result.
  auto it = local_.find(file);
  if (it != local_.end())
    return it->second == Ownership::Owns;

  // No result in local; we need to query global.
  bool did_insert = false;
  {
    std::lock_guard<std::mutex> lock(shared_->muetx);
    did_insert = shared_->files.insert(file).second;
  }
  local_[file] = did_insert ? Ownership::Owns : Ownership::DoesNotOwn;
  return did_insert;
}