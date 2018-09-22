// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "lsp_diagnostic.h"
#include "method.h"
#include "query.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

struct CompletionManager;
struct GroupMatch;
struct VFS;
struct Project;
struct WorkingFiles;
struct lsBaseOutMessage;

struct VFS {
  struct State {
    int64_t timestamp;
    int step;
    bool loaded;
  };
  std::unordered_map<std::string, State> state;
  std::mutex mutex;

  void Clear();
  bool Loaded(const std::string &path);
  bool Stamp(const std::string &path, int64_t ts, int step);
};

namespace ccls {
enum class IndexMode {
  NonInteractive,
  OnChange,
  Normal,
};

namespace pipeline {
extern int64_t loaded_ts, tick;
void Init();
void LaunchStdin();
void LaunchStdout();
void Indexer_Main(CompletionManager *completion, VFS *vfs, Project *project,
                  WorkingFiles *wfiles);
void MainLoop();

void Index(const std::string &path, const std::vector<const char *> &args,
           IndexMode mode, lsRequestId id = {});

std::optional<std::string> LoadIndexedContent(const std::string& path);
void WriteStdout(MethodType method, lsBaseOutMessage &response);
} // namespace pipeline
} // namespace ccls
