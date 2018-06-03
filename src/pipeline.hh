#pragma once

#include "method.h"
#include "query.h"

#include <string>
#include <unordered_map>
#include <vector>

class DiagnosticsPublisher;
struct VFS;
struct Project;
struct WorkingFiles;
struct lsBaseOutMessage;

namespace ccls::pipeline {

void Init();
void LaunchStdin();
void LaunchStdout();
void Indexer_Main(DiagnosticsPublisher* diag_pub,
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
