#include "file_consumer.h"

#include "indexer.h"
#include "platform.h"
#include "utils.h"

namespace {

std::string FileName(CXFile file) {
  CXString cx_name = clang_getFileName(file);
  std::string name = clang::ToString(cx_name);
  return NormalizePath(name);
}

}  // namespace

bool operator==(const CXFileUniqueID& a, const CXFileUniqueID& b) {
  return a.data[0] == b.data[0] && a.data[1] == b.data[1] && a.data[2] == b.data[2];
}

FileConsumer::FileConsumer(SharedState* shared_state) : shared_(shared_state) {}

IndexedFile* FileConsumer::TryConsumeFile(CXFile file, bool* is_first_ownership) {
  assert(is_first_ownership);

  CXFileUniqueID file_id;
  if (clang_getFileUniqueID(file, &file_id) != 0) {
    std::cerr << "Could not get unique file id for " << FileName(file) << std::endl;
    return nullptr;
  }

  // Try to find cached local result.
  auto it = local_.find(file_id);
  if (it != local_.end()) {
    *is_first_ownership = false;
    return it->second.get();
  }

  std::string file_name = FileName(file);

  // No result in local; we need to query global.
  bool did_insert = false;
  {
    std::lock_guard<std::mutex> lock(shared_->mutex);
    did_insert = shared_->files.insert(file_name).second;
  }
  *is_first_ownership = did_insert;
  local_[file_id] = did_insert ? MakeUnique<IndexedFile>(file_name) : nullptr;
  return local_[file_id].get();
}

IndexedFile* FileConsumer::ForceLocal(CXFile file) {
  CXFileUniqueID file_id;
  if (clang_getFileUniqueID(file, &file_id) != 0) {
    std::cerr << "Could not get unique file id for " << FileName(file) << std::endl;
    return nullptr;
  }

  auto it = local_.find(file_id);
  if (it == local_.end())
    local_[file_id] = MakeUnique<IndexedFile>(FileName(file));
  assert(local_.find(file_id) != local_.end());
  return local_[file_id].get();
}

std::vector<std::unique_ptr<IndexedFile>> FileConsumer::TakeLocalState() {
  std::vector<std::unique_ptr<IndexedFile>> result;
  for (auto& entry : local_) {
    if (entry.second)
      result.push_back(std::move(entry.second));
  }
  return result;
}