// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "lsp_diagnostic.h"
#include "method.h"
#include "query.h"

#include <string>
#include <unordered_map>
#include <vector>

struct CompletionManager;
struct GroupMatch;
struct VFS;
struct Project;
struct WorkingFiles;
struct lsBaseOutMessage;

class DiagnosticsPublisher {
  std::unique_ptr<GroupMatch> match_;
  int64_t nextPublish_ = 0;
  int frequencyMs_;

 public:
  void Init();
  void Publish(WorkingFiles* working_files,
               std::string path,
               std::vector<lsDiagnostic> diagnostics);
};

namespace ccls {
enum class IndexMode {
  NonInteractive,
  OnChange,
  Normal,
};

namespace pipeline {
void Init();
void LaunchStdin();
void LaunchStdout();
void Indexer_Main(CompletionManager *complete,
                  DiagnosticsPublisher *diag_pub, VFS *vfs, Project *project,
                  WorkingFiles *working_files);
void MainLoop();

void Index(const std::string &path, const std::vector<std::string> &args,
           IndexMode mode, lsRequestId id = {});

std::optional<int64_t> LastWriteTime(const std::string &path);
std::optional<std::string> LoadIndexedContent(const std::string& path);
void WriteStdout(MethodType method, lsBaseOutMessage &response);
} // namespace pipeline
} // namespace ccls
