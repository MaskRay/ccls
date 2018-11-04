/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#pragma once

#include "config.hh"
#include "lsp.hh"

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ccls {
struct WorkingFiles;

std::pair<LanguageId, bool> lookupExtension(std::string_view filename);

struct Project {
  struct Entry {
    std::string root;
    std::string directory;
    std::string filename;
    std::vector<const char *> args;
    // If true, this entry is inferred and was not read from disk.
    bool is_inferred = false;
    int id = -1;
  };

  struct Folder {
    std::string name;
    // Include directories for <> headers
    std::vector<std::string> angle_search_list;
    // Include directories for "" headers
    std::vector<std::string> quote_search_list;
    std::vector<Entry> entries;
    std::unordered_map<std::string, int> path2entry_index;
  };

  std::mutex mutex_;
  std::unordered_map<std::string, Folder> root2folder;

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
  void Load(const std::string &root_directory);

  // Lookup the CompilationEntry for |filename|. If no entry was found this
  // will infer one based on existing project structure.
  Entry FindEntry(const std::string &path, bool can_be_inferred);

  // If the client has overridden the flags, or specified them for a file
  // that is not in the compilation_database.json make sure those changes
  // are permanent.
  void SetArgsForFile(const std::vector<const char *> &args,
                      const std::string &path);

  void Index(WorkingFiles *wfiles, RequestId id);
};
} // namespace ccls
