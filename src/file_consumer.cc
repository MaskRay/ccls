/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "file_consumer.h"

#include "clang_utils.h"
#include "indexer.h"
#include "log.hh"
#include "platform.h"
#include "utils.h"

bool VFS::Loaded(const std::string &path) {
  std::lock_guard lock(mutex);
  return state[path].loaded;
}

bool VFS::Stamp(const std::string &path, int64_t ts, int step) {
  std::lock_guard<std::mutex> lock(mutex);
  State &st = state[path];
  if (st.timestamp < ts || (st.timestamp == ts && st.step < step)) {
    st.timestamp = ts;
    st.step = step;
    return true;
  } else
    return false;
}

FileConsumer::FileConsumer(VFS *vfs, const std::string &parse_file)
    : vfs_(vfs), parse_file_(parse_file) {}

IndexFile *FileConsumer::TryConsumeFile(
    const clang::FileEntry &File,
    const std::unordered_map<llvm::sys::fs::UniqueID, FileConsumer::File>
        &UID2File) {
  auto UniqueID = File.getUniqueID();
  {
    auto it = local_.find(UniqueID);
    if (it != local_.end())
      return it->second.get();
  }

  auto it = UID2File.find(UniqueID);
  assert(it != UID2File.end());
  assert(it->second.mtime);
  if (!vfs_->Stamp(it->second.path, it->second.mtime, 1)) {
    local_[UniqueID] = nullptr;
    return nullptr;
  }

  // Build IndexFile instance.
  local_[UniqueID] =
      std::make_unique<IndexFile>(UniqueID, it->second.path, it->second.content);
  return local_[UniqueID].get();
}

std::vector<std::unique_ptr<IndexFile>> FileConsumer::TakeLocalState() {
  std::vector<std::unique_ptr<IndexFile>> result;
  for (auto &entry : local_) {
    if (entry.second)
      result.push_back(std::move(entry.second));
  }
  return result;
}
