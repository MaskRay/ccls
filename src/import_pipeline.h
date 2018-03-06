#pragma once

// FIXME: do not include clang-c outside of clang_ files.
#include <clang-c/Index.h>

#include <atomic>
#include <memory>
#include <string>
#include <vector>

struct ClangTranslationUnit;
struct Config;
class DiagnosticsEngine;
struct FileConsumerSharedState;
struct ImportManager;
struct MultiQueueWaiter;
struct Project;
struct QueryDatabase;
struct SemanticHighlightSymbolCache;
struct TimestampManager;
struct WorkingFiles;

struct ImportPipelineStatus {
  std::atomic<int> num_active_threads;
  std::atomic<long long> next_progress_output;

  ImportPipelineStatus();
};

void IndexWithTuFromCodeCompletion(
    Config* config,
    FileConsumerSharedState* file_consumer_shared,
    ClangTranslationUnit* tu,
    const std::vector<CXUnsavedFile>& file_contents,
    const std::string& path,
    const std::vector<std::string>& args);

void Indexer_Main(Config* config,
                  DiagnosticsEngine* diag_engine,
                  FileConsumerSharedState* file_consumer_shared,
                  TimestampManager* timestamp_manager,
                  ImportManager* import_manager,
                  ImportPipelineStatus* status,
                  Project* project,
                  WorkingFiles* working_files,
                  MultiQueueWaiter* waiter);

bool QueryDb_ImportMain(Config* config,
                        QueryDatabase* db,
                        ImportManager* import_manager,
                        ImportPipelineStatus* status,
                        SemanticHighlightSymbolCache* semantic_cache,
                        WorkingFiles* working_files);
