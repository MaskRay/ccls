// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "config.h"
#include "lsp.h"

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

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

  void Index(WorkingFiles *wfiles, lsRequestId id);
};
