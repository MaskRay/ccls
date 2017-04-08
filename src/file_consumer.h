#pragma once

#include <mutex>
#include <unordered_set>
#include <unordered_map>

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
    mutable std::mutex muetx;
  };

  FileConsumer(SharedState* shared_state);

  // Returns true if this instance owns given |file|. This will also attempt to
  // take ownership over |file|.
  bool DoesOwnFile(const std::string& file);

  // Clear all ownership state.
  void ClearOwnership();

 private:
  enum class Ownership {
    Owns,
    DoesNotOwn
  };

  std::unordered_map<std::string, Ownership> local_;
  SharedState* shared_;
};