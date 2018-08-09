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
  return {0, 0, 0};
}

bool VFS::Mark(const std::string &file, int owner, int stage) {
  std::lock_guard<std::mutex> lock(mutex);
  State &st = state[file];
  if (st.stage < stage) {
    st.owner = owner;
    st.stage = stage;
    return true;
  } else
    return false;
}

bool VFS::Stamp(const std::string &file, int64_t ts) {
  std::lock_guard<std::mutex> lock(mutex);
  State &st = state[file];
  if (st.timestamp < ts) {
    st.timestamp = ts;
    return true;
  } else
    return false;
}

void VFS::ResetLocked(const std::string &file) {
  State &st = state[file];
  if (st.owner == 0 || st.owner == g_thread_id)
    st.stage = 0;
}

void VFS::Reset(const std::string &file) {
  std::lock_guard<std::mutex> lock(mutex);
  ResetLocked(file);
}

FileConsumer::FileConsumer(VFS *vfs, const std::string &parse_file)
    : vfs_(vfs), parse_file_(parse_file), thread_id_(g_thread_id) {}

IndexFile *FileConsumer::TryConsumeFile(
    const clang::FileEntry &File,
    std::unordered_map<std::string, FileContents> *file_contents_map) {
  auto UniqueID = File.getUniqueID();
  auto it = local_.find(UniqueID);
  if (it != local_.end())
    return it->second.get();

  std::string file_name = FileName(File);
  // We did not take the file from global. Cache that we failed so we don't try
  // again and return nullptr.
  if (!vfs_->Mark(file_name, thread_id_, 2)) {
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
