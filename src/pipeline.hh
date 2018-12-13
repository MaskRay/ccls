#pragma once

#include "lsp.hh"
#include "query.hh"

#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ccls {
struct SemaManager;
struct GroupMatch;
struct Project;
struct WorkingFiles;

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

enum class IndexMode {
  NonInteractive,
  OnChange,
  Normal,
};

namespace pipeline {
extern std::atomic<bool> quit;
extern std::atomic<int64_t> loaded_ts, pending_index_requests;
extern int64_t tick;

void ThreadEnter();
void ThreadLeave();
void Init();
void LaunchStdin();
void LaunchStdout();
void Indexer_Main(SemaManager *manager, VFS *vfs, Project *project,
                  WorkingFiles *wfiles);
void MainLoop();
void Standalone(const std::string &root);

void Index(const std::string &path, const std::vector<const char *> &args,
           IndexMode mode, RequestId id = {});

std::optional<std::string> LoadIndexedContent(const std::string& path);

void Notify(const char *method, const std::function<void(JsonWriter &)> &fn);
template <typename T> void Notify(const char *method, T &result) {
  Notify(method, [&](JsonWriter &w) { Reflect(w, result); });
}

void Reply(RequestId id, const std::function<void(JsonWriter &)> &fn);

void ReplyError(RequestId id, const std::function<void(JsonWriter &)> &fn);
template <typename T> void ReplyError(RequestId id, T &result) {
  ReplyError(id, [&](JsonWriter &w) { Reflect(w, result); });
}
} // namespace pipeline
} // namespace ccls
