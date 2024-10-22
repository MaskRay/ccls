// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

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
    // 0 unless coming from a compile_commands.json entry.
    int compdb_size = 0;
    int id = -1;
    int prio = 0;
  };

  struct Folder {
    std::string name;
    std::unordered_map<std::string, int> search_dir2kind;
    std::vector<Entry> entries;
    std::unordered_map<std::string, int> path2entry_index;
    std::unordered_map<std::string, std::vector<const char *>> dot_ccls;
  };

  std::mutex mtx;
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
  void load(const std::string &root);
  void loadDirectory(const std::string &root, Folder &folder);

  // Lookup the CompilationEntry for |filename|. If no entry was found this
  // will infer one based on existing project structure.
  Entry findEntry(const std::string &path, bool can_redirect, bool must_exist);

  // If the client has overridden the flags, or specified them for a file
  // that is not in the compilation_database.json make sure those changes
  // are permanent.
  void setArgsForFile(const std::vector<const char *> &args,
                      const std::string &path);

  void index(WorkingFiles *wfiles, const RequestId &id);
  void indexRelated(const std::string &path);
};
} // namespace ccls
