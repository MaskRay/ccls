#pragma once

// FIXME: do not include clang-c outside of clang_ files.
#include <clang-c/Index.h>

#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <mutex>
#include <unordered_map>
#include <vector>

struct ClangTranslationUnit;
class DiagnosticsEngine;
struct FileConsumerSharedState;
struct ICacheManager;
struct MultiQueueWaiter;
struct Project;
struct QueryDatabase;
struct SemanticHighlightSymbolCache;
struct WorkingFiles;

// Caches timestamps of cc files so we can avoid a filesystem reads. This is
// important for import perf, as during dependency checking the same files are
// checked over and over again if they are common headers.
struct TimestampManager {
  std::optional<int64_t> GetLastCachedModificationTime(ICacheManager* cache_manager,
                                                  const std::string& path);

  void UpdateCachedModificationTime(const std::string& path, int64_t timestamp);

  // TODO: use std::shared_mutex so we can have multiple readers.
  std::mutex mutex_;
  std::unordered_map<std::string, int64_t> timestamps_;
};

struct ImportPipelineStatus {
  std::atomic<int> num_active_threads;
  std::atomic<long long> next_progress_output;

  ImportPipelineStatus();
};

void IndexWithTuFromCodeCompletion(
    FileConsumerSharedState* file_consumer_shared,
    ClangTranslationUnit* tu,
    const std::vector<CXUnsavedFile>& file_contents,
    const std::string& path,
    const std::vector<std::string>& args);

void Indexer_Main(DiagnosticsEngine* diag_engine,
                  FileConsumerSharedState* file_consumer_shared,
                  TimestampManager* timestamp_manager,
                  ImportPipelineStatus* status,
                  Project* project,
                  WorkingFiles* working_files,
                  MultiQueueWaiter* waiter);

bool QueryDb_ImportMain(QueryDatabase* db,
                        ImportPipelineStatus* status,
                        SemanticHighlightSymbolCache* semantic_cache,
                        WorkingFiles* working_files);
