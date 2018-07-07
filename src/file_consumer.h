#pragma once

#include "position.h"
#include "serializer.h"
#include "utils.h"

#include <clang-c/Index.h>
#include <clang/Basic/FileManager.h>

#include <functional>
#include <map>
#include <mutex>
#include <vector>

struct IndexFile;

struct FileContents {
  FileContents() = default;
  FileContents(const std::string& path, const std::string& content);

  std::optional<int> ToOffset(Position p) const;
  std::optional<std::string> ContentsInRange(Range range) const;

  std::string path;
  std::string content;
  // {0, 1 + position of first newline, 1 + position of second newline, ...}
  std::vector<int> line_offsets_;
};

struct VFS {
  struct State {
    int64_t timestamp;
    int owner;
    int stage;
  };
  mutable std::unordered_map<std::string, State> state;
  mutable std::mutex mutex;

  State Get(const std::string& file);
  bool Mark(const std::string& file, int owner, int stage);
  bool Stamp(const std::string& file, int64_t ts);
  void ResetLocked(const std::string& file);
  void Reset(const std::string& file);
};

// FileConsumer is used by the indexer. When it encouters a file, it tries to
// take ownership over it. If the indexer has ownership over a file, it will
// produce an index, otherwise, it will emit nothing for that declarations
// and references coming from that file.
//
// The indexer does this because header files do not have their own translation
// units but we still want to index them.
struct FileConsumer {
  FileConsumer(VFS* vfs, const std::string& parse_file);

  // Returns IndexFile for the file or nullptr. |is_first_ownership| is set
  // to true iff the function just took ownership over the file. Otherwise it
  // is set to false.
  //
  // note: file_contents is passed as a parameter instead of as a member
  // variable since it is large and we do not want to copy it.
  IndexFile* TryConsumeFile(const clang::FileEntry& file,
                            std::unordered_map<std::string, FileContents>* file_contents);

  // Returns and passes ownership of all local state.
  std::vector<std::unique_ptr<IndexFile>> TakeLocalState();

 private:
  std::map<llvm::sys::fs::UniqueID, std::unique_ptr<IndexFile>> local_;
  VFS* vfs_;
  std::string parse_file_;
  int thread_id_;
};
