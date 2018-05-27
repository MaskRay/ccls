#pragma once

#include "queue_manager.h"
#include "timer.h"

#include <atomic>
#include <unordered_map>

struct ClangTranslationUnit;
class DiagnosticsEngine;
struct VFS;
struct ICacheManager;
struct Project;
struct QueryDatabase;
struct SemanticHighlightSymbolCache;
struct WorkingFiles;

void Indexer_Main(DiagnosticsEngine* diag_engine,
                  VFS* vfs,
                  Project* project,
                  WorkingFiles* working_files,
                  MultiQueueWaiter* waiter);

void LaunchStdinLoop(std::unordered_map<MethodType, Timer>* request_times);
void LaunchStdoutThread(std::unordered_map<MethodType, Timer>* request_times,
                        MultiQueueWaiter* waiter);
void MainLoop(MultiQueueWaiter* querydb_waiter,
              MultiQueueWaiter* indexer_waiter);
