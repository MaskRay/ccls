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

namespace {

std::optional<std::string>
GetFileContents(const std::string &path,
                std::unordered_map<std::string, FileContents> *file_contents) {
  auto it = file_contents->find(path);
  if (it == file_contents->end()) {
    std::optional<std::string> content = ReadContent(path);
    if (content)
      (*file_contents)[path] = FileContents(path, *content);
    return content;
  }
  return it->second.content;
}

} // namespace

FileContents::FileContents(const std::string &path, const std::string &content)
    : path(path), content(content) {
  line_offsets_.push_back(0);
  for (size_t i = 0; i < content.size(); i++) {
    if (content[i] == '\n')
      line_offsets_.push_back(i + 1);
  }
}

std::optional<int> FileContents::ToOffset(Position p) const {
  if (0 <= p.line && size_t(p.line) < line_offsets_.size()) {
    int ret = line_offsets_[p.line] + p.column;
    if (size_t(ret) < content.size())
      return ret;
  }
  return std::nullopt;
}

std::optional<std::string> FileContents::ContentsInRange(Range range) const {
  std::optional<int> start_offset = ToOffset(range.start),
                     end_offset = ToOffset(range.end);
  if (start_offset && end_offset && *start_offset < *end_offset)
    return content.substr(*start_offset, *end_offset - *start_offset);
  return std::nullopt;
}

VFS::State VFS::Get(const std::string &file) {
  std::lock_guard<std::mutex> lock(mutex);
  auto it = state.find(file);
  if (it != state.end())
    return it->second;
  return {0, 0};
}

bool VFS::Stamp(const std::string &file, int64_t ts, int64_t offset) {
  std::lock_guard<std::mutex> lock(mutex);
  State &st = state[file];
  if (st.timestamp < ts) {
    st.timestamp = ts + offset;
    return true;
  } else
    return false;
}

FileConsumer::FileConsumer(VFS *vfs, const std::string &parse_file)
    : vfs_(vfs), parse_file_(parse_file) {}

IndexFile *FileConsumer::TryConsumeFile(
    const clang::FileEntry &File,
    std::unordered_map<std::string, FileContents> *file_contents_map) {
  auto UniqueID = File.getUniqueID();
  auto it = local_.find(UniqueID);
  if (it != local_.end())
    return it->second.get();

  std::string file_name = FileName(File);
  int64_t tim = File.getModificationTime();
  assert(tim);
  if (!vfs_->Stamp(file_name, tim, 0)) {
    local_[UniqueID] = nullptr;
    return nullptr;
  }

  // Read the file contents, if we fail then we cannot index the file.
  std::optional<std::string> contents =
      GetFileContents(file_name, file_contents_map);
  if (!contents)
    return nullptr;

  // Build IndexFile instance.
  local_[UniqueID] =
      std::make_unique<IndexFile>(UniqueID, file_name, *contents);
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
