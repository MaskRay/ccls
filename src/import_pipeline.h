#pragma once

#include <atomic>

struct ClangTranslationUnit;
class DiagnosticsEngine;
struct VFS;
struct ICacheManager;
struct MultiQueueWaiter;
struct Project;
struct QueryDatabase;
struct SemanticHighlightSymbolCache;
struct WorkingFiles;

struct ImportPipelineStatus {
  std::atomic<int> num_active_threads = {0};
  std::atomic<long long> next_progress_output = {0};
};

void Indexer_Main(DiagnosticsEngine* diag_engine,
                  VFS* vfs,
                  ImportPipelineStatus* status,
                  Project* project,
                  WorkingFiles* working_files,
                  MultiQueueWaiter* waiter);

bool QueryDb_ImportMain(QueryDatabase* db,
                        ImportPipelineStatus* status,
                        SemanticHighlightSymbolCache* semantic_cache,
                        WorkingFiles* working_files);
