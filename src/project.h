#pragma once

#include <optional.h>
#include <sparsepp/spp.h>

#include <mutex>
#include <string>
#include <vector>

using std::experimental::optional;
using std::experimental::nullopt;

struct Project {
  struct Entry {
    std::string filename;
    std::vector<std::string> args;
    
    std::string import_file;
    std::vector<std::string> import_dependencies;
    optional<uint64_t> last_modification_time;
  };

  std::vector<Entry> entries;
  spp::sparse_hash_map<std::string, int> absolute_path_to_entry_index_;
  std::mutex entries_modification_mutex_;

  // Loads a project for the given |directory|.
  //
  // If |directory| contains a compile_commands.json file, that will be used to
  // discover all files and args. Otherwise, a recursive directory listing of
  // all *.cpp, *.cc, *.h, and *.hpp files will be used. clang arguments can be
  // specified in a clang_args file located inside of |directory|.
  void Load(const std::string& directory);

  // Lookup the CompilationEntry for |filename|.
  optional<Entry> FindCompilationEntryForFile(const std::string& filename);

  // Update the modification time for the given filename. This is thread-safe.
  void UpdateFileState(const std::string& filename, const std::string& import_file, const std::vector<std::string>& import_dependencies, uint64_t modification_time);
};

