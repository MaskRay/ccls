// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "clang_tu.hh"
#include "lsp.hh"
#include "project.hh"
#include "threaded_queue.hh"
#include "working_files.hh"

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Sema/CodeCompleteOptions.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace ccls {
struct PreambleData;

struct DiagBase {
  Range range;
  std::string message;
  std::string file;
  clang::DiagnosticsEngine::Level level = clang::DiagnosticsEngine::Note;
  unsigned category;
  bool concerned = false;
};
struct Note : DiagBase {};
struct Diag : DiagBase {
  std::vector<Note> notes;
  std::vector<TextEdit> edits;
};

TextEdit toTextEdit(const clang::SourceManager &SM, const clang::LangOptions &L,
                    const clang::FixItHint &FixIt);

template <typename K, typename V> struct LruCache {
  std::shared_ptr<V> get(const K &key) {
    for (auto it = items.begin(); it != items.end(); ++it)
      if (it->first == key) {
        auto x = std::move(*it);
        std::move_backward(items.begin(), it, it + 1);
        items[0] = std::move(x);
        return items[0].second;
      }
    return nullptr;
  }
  std::shared_ptr<V> take(const K &key) {
    for (auto it = items.begin(); it != items.end(); ++it)
      if (it->first == key) {
        auto x = std::move(it->second);
        items.erase(it);
        return x;
      }
    return nullptr;
  }
  void insert(const K &key, std::shared_ptr<V> value) {
    if ((int)items.size() >= capacity)
      items.pop_back();
    items.emplace(items.begin(), key, std::move(value));
  }
  void clear() { items.clear(); }
  void setCapacity(int cap) { capacity = cap; }

private:
  std::vector<std::pair<K, std::shared_ptr<V>>> items;
  int capacity = 1;
};

struct Session {
  std::mutex mutex;
  std::shared_ptr<PreambleData> preamble;

  Project::Entry file;
  WorkingFiles *wfiles;
  bool inferred = false;

  // TODO share
  llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> fs =
      llvm::vfs::getRealFileSystem();
  std::shared_ptr<clang::PCHContainerOperations> pch;

  Session(const Project::Entry &file, WorkingFiles *wfiles,
          std::shared_ptr<clang::PCHContainerOperations> pch)
      : file(file), wfiles(wfiles), pch(pch) {}

  std::shared_ptr<PreambleData> getPreamble();
};

struct SemaManager {
  using OnDiagnostic = std::function<void(std::string path,
                                          std::vector<Diagnostic> diagnostics)>;
  // If OptConsumer is nullptr, the request has been cancelled.
  using OnComplete =
      std::function<void(clang::CodeCompleteConsumer *OptConsumer)>;
  using OnDropped = std::function<void(RequestId request_id)>;

  struct CompTask {
    CompTask(const RequestId &id, const std::string &path,
             const Position &position,
             std::unique_ptr<clang::CodeCompleteConsumer> Consumer,
             clang::CodeCompleteOptions CCOpts, const OnComplete &on_complete)
        : id(id), path(path), position(position), consumer(std::move(Consumer)),
          cc_opts(CCOpts), on_complete(on_complete) {}

    RequestId id;
    std::string path;
    Position position;
    std::unique_ptr<clang::CodeCompleteConsumer> consumer;
    clang::CodeCompleteOptions cc_opts;
    OnComplete on_complete;
  };
  struct DiagTask {
    std::string path;
    int64_t wait_until;
    int64_t debounce;
  };
  struct PreambleTask {
    std::string path;
    std::unique_ptr<CompTask> comp_task;
    bool from_diag = false;
  };

  SemaManager(Project *project, WorkingFiles *wfiles,
              OnDiagnostic on_diagnostic, OnDropped on_dropped);

  void scheduleDiag(const std::string &path, int debounce);
  void onView(const std::string &path);
  void onSave(const std::string &path);
  void onClose(const std::string &path);
  std::shared_ptr<ccls::Session> ensureSession(const std::string &path,
                                               bool *created = nullptr);
  void clear();
  void quit();

  // Global state.
  Project *project_;
  WorkingFiles *wfiles;
  OnDiagnostic on_diagnostic_;
  OnDropped on_dropped_;

  std::mutex mutex;
  LruCache<std::string, ccls::Session> sessions;

  std::mutex diag_mutex;
  std::unordered_map<std::string, int64_t> next_diag;

  ThreadedQueue<std::unique_ptr<CompTask>> comp_tasks;
  ThreadedQueue<DiagTask> diag_tasks;
  ThreadedQueue<PreambleTask> preamble_tasks;

  std::shared_ptr<clang::PCHContainerOperations> pch;
};

// Cached completion information, so we can give fast completion results when
// the user erases a character. vscode will resend the completion request if
// that happens.
template <typename T> struct CompleteConsumerCache {
  std::mutex mutex;
  std::string path;
  Position position;
  T result;

  template <typename Fn> void withLock(Fn &&fn) {
    std::lock_guard lock(mutex);
    fn();
  }
  bool isCacheValid(const std::string path, Position position) {
    std::lock_guard lock(mutex);
    return this->path == path && this->position == position;
  }
};
} // namespace ccls
