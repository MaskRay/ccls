#pragma once

#include "config.h"

#include <optional.h>
#include <sparsepp/spp.h>

#include <functional>
#include <mutex>
#include <string>
#include <vector>

using std::experimental::nullopt;
using std::experimental::optional;

struct Project {
  struct Entry {
    std::string filename;
    std::vector<std::string> args;
    // If true, this entry is inferred and was not read from disk.
    bool is_inferred = false;
  };

  // Include directories for "" headers
  std::vector<std::string> quote_include_directories;
  // Include directories for <> headers
  std::vector<std::string> angle_include_directories;

  std::vector<Entry> entries;
  spp::sparse_hash_map<std::string, int> absolute_path_to_entry_index_;

  // Loads a project for the given |directory|.
  //
  // If |directory| contains a compile_commands.json file, that will be used to
  // discover all files and args. Otherwise, a recursive directory listing of
  // all *.cpp, *.cc, *.h, and *.hpp files will be used. clang arguments can be
  // specified in a clang_args file located inside of |directory|.
  void Load(const std::vector<std::string>& extra_flags,
            const std::string& directory);

  // Lookup the CompilationEntry for |filename|. If no entry was found this
  // will infer one based on existing project structure.
  Entry FindCompilationEntryForFile(const std::string& filename);

  void ForAllFilteredFiles(
      Config* config,
      std::function<void(int i, const Entry& entry)> action);
};
