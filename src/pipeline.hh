#pragma once

#include "method.h"
#include "query.h"
#include "timer.h"

#include <string>
#include <unordered_map>
#include <vector>

class DiagnosticsEngine;
struct VFS;
struct Project;
struct WorkingFiles;
struct lsBaseOutMessage;

namespace ccls::pipeline {

void Init();
void LaunchStdin(std::unordered_map<MethodType, Timer>* request_times);
void LaunchStdout(std::unordered_map<MethodType, Timer>* request_times);
void Indexer_Main(DiagnosticsEngine* diag_engine,
                  VFS* vfs,
                  Project* project,
                  WorkingFiles* working_files);
void MainLoop();

void Index(const std::string& path,
           const std::vector<std::string>& args,
           bool is_interactive,
           lsRequestId id = {});

std::optional<std::string> LoadCachedFileContents(const std::string& path);
void WriteStdout(MethodType method, lsBaseOutMessage& response);
}
