// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

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
    int loaded;
  };
  std::unordered_map<std::string, State> state;
  std::mutex mutex;

  void clear();
  int loaded(const std::string &path);
  bool stamp(const std::string &path, int64_t ts, int step);
};

enum class IndexMode {
  Delete,
  Background,
  OnChange,
  Normal,
};

struct IndexStats {
  std::atomic<int64_t> last_idle, completed, enqueued;
};

namespace pipeline {
extern std::atomic<bool> g_quit;
extern std::atomic<int64_t> loaded_ts;
extern IndexStats stats;
extern int64_t tick;

void threadEnter();
void threadLeave();
void init();
void launchStdin();
void launchStdout();
void indexer_Main(SemaManager *manager, VFS *vfs, Project *project,
                  WorkingFiles *wfiles);
void mainLoop();
void standalone(const std::string &root);

void index(const std::string &path, const std::vector<const char *> &args,
           IndexMode mode, bool must_exist, RequestId id = {});
void removeCache(const std::string &path);
std::optional<std::string> loadIndexedContent(const std::string &path);

void notifyOrRequest(const char *method, bool request,
                     const std::function<void(JsonWriter &)> &fn);
template <typename T> void notify(const char *method, T &result) {
  notifyOrRequest(method, false, [&](JsonWriter &w) { reflect(w, result); });
}
template <typename T> void request(const char *method, T &result) {
  notifyOrRequest(method, true, [&](JsonWriter &w) { reflect(w, result); });
}

void reply(const RequestId &id, const std::function<void(JsonWriter &)> &fn);

void replyError(const RequestId &id,
                const std::function<void(JsonWriter &)> &fn);
template <typename T> void replyError(const RequestId &id, T &result) {
  replyError(id, [&](JsonWriter &w) { reflect(w, result); });
}
} // namespace pipeline
} // namespace ccls
