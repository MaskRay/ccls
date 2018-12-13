/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

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

TextEdit ToTextEdit(const clang::SourceManager &SM,
                      const clang::LangOptions &L,
                      const clang::FixItHint &FixIt);

template <typename K, typename V> struct LruCache {
  std::shared_ptr<V> Get(const K &key) {
    for (auto it = items.begin(); it != items.end(); ++it)
      if (it->first == key) {
        auto x = std::move(*it);
        std::move_backward(items.begin(), it, it + 1);
        items[0] = std::move(x);
        return items[0].second;
      }
    return nullptr;
  }
  std::shared_ptr<V> Take(const K &key) {
    for (auto it = items.begin(); it != items.end(); ++it)
      if (it->first == key) {
        auto x = std::move(it->second);
        items.erase(it);
        return x;
      }
    return nullptr;
  }
  void Insert(const K &key, std::shared_ptr<V> value) {
    if ((int)items.size() >= capacity)
      items.pop_back();
    items.emplace(items.begin(), key, std::move(value));
  }
  void Clear() { items.clear(); }
  void SetCapacity(int cap) { capacity = cap; }

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
  llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> FS =
      llvm::vfs::getRealFileSystem();
  std::shared_ptr<clang::PCHContainerOperations> PCH;

  Session(const Project::Entry &file, WorkingFiles *wfiles,
                    std::shared_ptr<clang::PCHContainerOperations> PCH)
      : file(file), wfiles(wfiles), PCH(PCH) {}

  std::shared_ptr<PreambleData> GetPreamble();
};

struct SemaManager {
  using OnDiagnostic = std::function<void(
      std::string path, std::vector<Diagnostic> diagnostics)>;
  // If OptConsumer is nullptr, the request has been cancelled.
  using OnComplete =
      std::function<void(clang::CodeCompleteConsumer *OptConsumer)>;
  using OnDropped = std::function<void(RequestId request_id)>;

  struct PreambleTask {
    std::string path;
    bool from_diag = false;
  };
  struct CompTask {
    CompTask(const RequestId &id, const std::string &path,
             const Position &position,
             std::unique_ptr<clang::CodeCompleteConsumer> Consumer,
             clang::CodeCompleteOptions CCOpts, const OnComplete &on_complete)
        : id(id), path(path), position(position), Consumer(std::move(Consumer)),
          CCOpts(CCOpts), on_complete(on_complete) {}

    RequestId id;
    std::string path;
    Position position;
    std::unique_ptr<clang::CodeCompleteConsumer> Consumer;
    clang::CodeCompleteOptions CCOpts;
    OnComplete on_complete;
  };
  struct DiagTask {
    std::string path;
    int64_t wait_until;
    int64_t debounce;
  };

  SemaManager(Project *project, WorkingFiles *wfiles,
                    OnDiagnostic on_diagnostic, OnDropped on_dropped);

  void ScheduleDiag(const std::string &path, int debounce);
  void OnView(const std::string &path);
  void OnSave(const std::string &path);
  void OnClose(const std::string &path);
  std::shared_ptr<ccls::Session> EnsureSession(const std::string &path,
                                               bool *created = nullptr);
  void Clear();
  void Quit();

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

  std::shared_ptr<clang::PCHContainerOperations> PCH;
};

// Cached completion information, so we can give fast completion results when
// the user erases a character. vscode will resend the completion request if
// that happens.
template <typename T>
struct CompleteConsumerCache {
  std::mutex mutex;
  std::string path;
  Position position;
  T result;

  template <typename Fn> void WithLock(Fn &&fn) {
    std::lock_guard lock(mutex);
    fn();
  }
  bool IsCacheValid(const std::string path, Position position) {
    std::lock_guard lock(mutex);
    return this->path == path && this->position == position;
  }
};
} // namespace ccls
