#pragma once

#include <optional.h>
#include <string>
#include <vector>

using std::experimental::optional;
using std::experimental::nullopt;

struct Project {
  struct Entry {
    std::string filename;
    std::vector<std::string> args;
    // optional<uint64_t> last_modification_time;
  };

  std::vector<Entry> entries;

  // Loads a project for the given |directory|.
  //
  // If |directory| contains a compile_commands.json file, that will be used to
  // discover all files and args. Otherwise, a recursive directory listing of
  // all *.cpp, *.cc, *.h, and *.hpp files will be used. clang arguments can be
  // specified in a clang_args file located inside of |directory|.
  void Load(const std::string& directory);

  // Lookup the CompilationEntry for |filename|.
  optional<Entry> FindCompilationEntryForFile(const std::string& filename);
};

