#pragma once

#include "config.h"
#include "method.h"

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class QueueManager;
struct WorkingFiles;

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
  std::unordered_map<std::string, int> absolute_path_to_entry_index_;

  // Loads a project for the given |directory|.
  //
  // If |config->compilationDatabaseDirectory| is not empty, look for .ccls or
  // compile_commands.json in it, otherwise they are retrieved in
  // |root_directory|.
  // For .ccls, recursive directory listing is used and files with known
  // suffixes are indexed. .ccls files can exist in subdirectories and they
  // will affect flags in their subtrees (relative paths are relative to the
  // project root, not subdirectories). For compile_commands.json, its entries
  // are indexed.
  void Load(const std::string& root_directory);

  // Lookup the CompilationEntry for |filename|. If no entry was found this
  // will infer one based on existing project structure.
  Entry FindCompilationEntryForFile(const std::string& filename);

  // If the client has overridden the flags, or specified them for a file
  // that is not in the compilation_database.json make sure those changes
  // are permanent.
  void SetFlagsForFile(
      const std::vector<std::string>& flags,
      const std::string& path);

  // Run |action| on every file in the project.
  void ForAllFilteredFiles(
      std::function<void(int i, const Entry& entry)> action);

  void Index(QueueManager* queue, WorkingFiles* wfiles, lsRequestId id);
};
