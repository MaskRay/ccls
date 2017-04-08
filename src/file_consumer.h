#pragma once

#include <mutex>
#include <unordered_set>
#include <unordered_map>

struct IndexedFile;

// FileConsumer is used by the indexer. When it encouters a file, it tries to
// take ownership over it. If the indexer has ownership over a file, it will
// produce an index, otherwise, it will emit nothing for that declarations
// and references coming from that file.
//
// The indexer does this because header files do not have their own translation
// units but we still want to index them.
struct FileConsumer {
  struct SharedState {
    mutable std::unordered_set<std::string> files;
    mutable std::mutex mutex;
  };

  FileConsumer(SharedState* shared_state);

  // Returns true if this instance owns given |file|. This will also attempt to
  // take ownership over |file|.
  //
  // Returns IndexedFile for the file or nullptr.
  IndexedFile* TryConsumeFile(const std::string& file);

  // Returns and passes ownership of all local state.
  std::vector<std::unique_ptr<IndexedFile>> TakeLocalState();

 private:
  std::unordered_map<std::string, std::unique_ptr<IndexedFile>> local_;
  SharedState* shared_;
};