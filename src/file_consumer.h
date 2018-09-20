// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "position.h"
#include "serializer.h"
#include "utils.h"

#include <clang/Basic/FileManager.h>

#include <mutex>
#include <unordered_map>
#include <vector>

struct IndexFile;

struct VFS {
  struct State {
    int64_t timestamp;
    int step;
    bool loaded;
  };
  mutable std::unordered_map<std::string, State> state;
  mutable std::mutex mutex;

  bool Loaded(const std::string &path);
  bool Stamp(const std::string &path, int64_t ts, int step);
};

namespace std {
template <> struct hash<llvm::sys::fs::UniqueID> {
  std::size_t operator()(llvm::sys::fs::UniqueID ID) const {
    size_t ret = ID.getDevice();
    hash_combine(ret, ID.getFile());
    return ret;
  }
};
} // namespace std

// FileConsumer is used by the indexer. When it encouters a file, it tries to
// take ownership over it. If the indexer has ownership over a file, it will
// produce an index, otherwise, it will emit nothing for that declarations
// and references coming from that file.
//
// The indexer does this because header files do not have their own translation
// units but we still want to index them.
struct FileConsumer {
  struct File {
    std::string path;
    int64_t mtime;
    std::string content;
  };

  FileConsumer(VFS *vfs, const std::string &parse_file);

  // Returns IndexFile or nullptr for the file or nullptr.
  IndexFile *TryConsumeFile(
      const clang::FileEntry &file,
      const std::unordered_map<llvm::sys::fs::UniqueID, File> &);

  // Returns and passes ownership of all local state.
  std::vector<std::unique_ptr<IndexFile>> TakeLocalState();

private:
  std::unordered_map<llvm::sys::fs::UniqueID, std::unique_ptr<IndexFile>>
      local_;
  VFS *vfs_;
  std::string parse_file_;
};
