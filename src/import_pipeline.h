#pragma once

#include "file_consumer.h"

#include <atomic>
#include <string>
#include <vector>

struct ClangTranslationUnit;
struct Config;
struct ImportManager;
struct MultiQueueWaiter;
struct Project;
struct QueryDatabase;
struct SemanticHighlightSymbolCache;
struct TimestampManager;
struct WorkingFiles;

struct ImportPipelineStatus {
  std::atomic<int> num_active_threads;

  ImportPipelineStatus();
};

void IndexWithTuFromCodeCompletion(
    FileConsumer::SharedState* file_consumer_shared,
    ClangTranslationUnit* tu,
    const std::vector<CXUnsavedFile>& file_contents,
    const std::string& path,
    const std::vector<std::string>& args);

void IndexMain(Config* config,
               FileConsumer::SharedState* file_consumer_shared,
               TimestampManager* timestamp_manager,
               ImportManager* import_manager,
               ImportPipelineStatus* status,
               Project* project,
               WorkingFiles* working_files,
               MultiQueueWaiter* waiter);

bool QueryDb_ImportMain(Config* config,
                        QueryDatabase* db,
                        ImportManager* import_manager,
                        SemanticHighlightSymbolCache* semantic_cache,
                        WorkingFiles* working_files);
