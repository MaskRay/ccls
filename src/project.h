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
  };

  std::vector<Entry> entries;
  spp::sparse_hash_map<std::string, int> absolute_path_to_entry_index_;

  // Loads a project for the given |directory|.
  //
  // If |directory| contains a compile_commands.json file, that will be used to
  // discover all files and args. Otherwise, a recursive directory listing of
  // all *.cpp, *.cc, *.h, and *.hpp files will be used. clang arguments can be
  // specified in a clang_args file located inside of |directory|.
  void Load(const std::string& directory);

  // Lookup the CompilationEntry for |filename|.
  optional<Entry> FindCompilationEntryForFile(const std::string& filename);

  // Helper that uses FindCompilationEntryForFile.
  optional<std::vector<std::string>> FindArgsForFile(const std::string& filename);
};

