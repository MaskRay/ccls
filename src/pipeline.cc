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

#include "pipeline.hh"

#include "config.hh"
#include "include_complete.hh"
#include "log.hh"
#include "lsp.hh"
#include "message_handler.hh"
#include "pipeline.hh"
#include "platform.hh"
#include "project.hh"
#include "query.hh"
#include "sema_manager.hh"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>

#include <llvm/Support/Process.h>
#include <llvm/Support/Threading.h>
using namespace llvm;

#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <thread>
#ifndef _WIN32
#include <unistd.h>
#endif

namespace ccls {
namespace {
struct PublishDiagnosticParam {
  DocumentUri uri;
  std::vector<Diagnostic> diagnostics;
};
REFLECT_STRUCT(PublishDiagnosticParam, uri, diagnostics);
} // namespace

void VFS::Clear() {
  std::lock_guard lock(mutex);
  state.clear();
}

bool VFS::Loaded(const std::string &path) {
  std::lock_guard lock(mutex);
  return state[path].loaded;
}

bool VFS::Stamp(const std::string &path, int64_t ts, int step) {
  std::lock_guard<std::mutex> lock(mutex);
  State &st = state[path];
  if (st.timestamp < ts || (st.timestamp == ts && st.step < step)) {
    st.timestamp = ts;
    st.step = step;
    return true;
  } else
    return false;
}

struct MessageHandler;
void StandaloneInitialize(MessageHandler &, const std::string &root);

namespace pipeline {

std::atomic<bool> quit;
std::atomic<int64_t> loaded_ts{0}, pending_index_requests{0}, request_id{0};
int64_t tick = 0;

namespace {

struct IndexRequest {
  std::string path;
  std::vector<const char *> args;
  IndexMode mode;
  bool must_exist = false;
  RequestId id;
  int64_t ts = tick++;
};

std::mutex thread_mtx;
std::condition_variable no_active_threads;
int active_threads;

MultiQueueWaiter *main_waiter;
MultiQueueWaiter *indexer_waiter;
MultiQueueWaiter *stdout_waiter;
ThreadedQueue<InMessage> *on_request;
ThreadedQueue<IndexRequest> *index_request;
ThreadedQueue<IndexUpdate> *on_indexed;
ThreadedQueue<std::string> *for_stdout;

struct InMemoryIndexFile {
  std::string content;
  IndexFile index;
};
std::shared_mutex g_index_mutex;
std::unordered_map<std::string, InMemoryIndexFile> g_index;

bool CacheInvalid(VFS *vfs, IndexFile *prev, const std::string &path,
                  const std::vector<const char *> &args,
                  const std::optional<std::string> &from) {
  {
    std::lock_guard<std::mutex> lock(vfs->mutex);
    if (prev->mtime < vfs->state[path].timestamp) {
      LOG_S(INFO) << "timestamp changed for " << path
                  << (from ? " (via " + *from + ")" : std::string());
      return true;
    }
  }

  bool changed = prev->args.size() != args.size();
  for (size_t i = 0; !changed && i < args.size(); i++)
    if (strcmp(prev->args[i], args[i]))
      changed = true;
  if (changed)
    LOG_S(INFO) << "args changed for " << path
                << (from ? " (via " + *from + ")" : std::string());
  return changed;
};

std::string AppendSerializationFormat(const std::string &base) {
  switch (g_config->cacheFormat) {
  case SerializeFormat::Binary:
    return base + ".blob";
  case SerializeFormat::Json:
    return base + ".json";
  }
}

std::string GetCachePath(const std::string &source_file) {
  for (auto &root : g_config->workspaceFolders)
    if (StringRef(source_file).startswith(root)) {
      auto len = root.size();
      return g_config->cacheDirectory +
             EscapeFileName(root.substr(0, len - 1)) + '/' +
             EscapeFileName(source_file.substr(len));
    }
  return g_config->cacheDirectory + '@' +
         EscapeFileName(g_config->fallbackFolder.substr(
             0, g_config->fallbackFolder.size() - 1)) +
         '/' + EscapeFileName(source_file);
}

std::unique_ptr<IndexFile> RawCacheLoad(const std::string &path) {
  if (g_config->cacheDirectory.empty()) {
    std::shared_lock lock(g_index_mutex);
    auto it = g_index.find(path);
    if (it == g_index.end())
      return nullptr;
    return std::make_unique<IndexFile>(it->second.index);
  }

  std::string cache_path = GetCachePath(path);
  std::optional<std::string> file_content = ReadContent(cache_path);
  std::optional<std::string> serialized_indexed_content =
      ReadContent(AppendSerializationFormat(cache_path));
  if (!file_content || !serialized_indexed_content)
    return nullptr;

  return ccls::Deserialize(g_config->cacheFormat, path,
                           *serialized_indexed_content, *file_content,
                           IndexFile::kMajorVersion);
}

std::mutex &GetFileMutex(const std::string &path) {
  const int N_MUTEXES = 256;
  static std::mutex mutexes[N_MUTEXES];
  return mutexes[std::hash<std::string>()(path) % N_MUTEXES];
}

bool Indexer_Parse(SemaManager *completion, WorkingFiles *wfiles,
                   Project *project, VFS *vfs, const GroupMatch &matcher) {
  std::optional<IndexRequest> opt_request = index_request->TryPopFront();
  if (!opt_request)
    return false;
  auto &request = *opt_request;
  bool loud = request.mode != IndexMode::OnChange;
  struct RAII {
    ~RAII() { pending_index_requests--; }
  } raii;

  // Dummy one to trigger refresh semantic highlight.
  if (request.path.empty()) {
    IndexUpdate dummy;
    dummy.refresh = true;
    on_indexed->PushBack(std::move(dummy), false);
    return false;
  }

  if (!matcher.Matches(request.path)) {
    LOG_IF_S(INFO, loud) << "skip " << request.path;
    return false;
  }

  // must_exist is currently unused.
  Project::Entry entry = project->FindEntry(request.path, false);
  if (request.must_exist && entry.filename.empty())
    return true;
  if (request.args.size())
    entry.args = request.args;
  std::string path_to_index = entry.filename;
  std::unique_ptr<IndexFile> prev;

  bool deleted = false;
  int reparse = 0;
  std::optional<int64_t> write_time = LastWriteTime(path_to_index);
  if (!write_time) {
    deleted = true;
  } else {
    reparse = vfs->Stamp(path_to_index, *write_time, 0);
    if (request.path != path_to_index) {
      std::optional<int64_t> mtime1 = LastWriteTime(request.path);
      if (!mtime1)
        deleted = true;
      else if (vfs->Stamp(request.path, *mtime1, 0))
        reparse = 2;
    }
  }
  if (deleted)
    reparse = 2;

  if (g_config->index.onChange) {
    reparse = 2;
    std::lock_guard lock(vfs->mutex);
    vfs->state[path_to_index].step = 0;
    if (request.path != path_to_index)
      vfs->state[request.path].step = 0;
  }
  bool track = g_config->index.trackDependency > 1 ||
               (g_config->index.trackDependency == 1 && request.ts < loaded_ts);
  if (!reparse && !track)
    return true;

  if (reparse < 2)
    do {
      std::unique_lock lock(GetFileMutex(path_to_index));
      prev = RawCacheLoad(path_to_index);
      if (!prev || CacheInvalid(vfs, prev.get(), path_to_index, entry.args,
          std::nullopt))
        break;
      if (track)
        for (const auto &dep : prev->dependencies) {
          if (auto mtime1 = LastWriteTime(dep.first.val().str())) {
            if (dep.second < *mtime1) {
              reparse = 2;
              break;
            }
          } else {
            reparse = 2;
            break;
          }
        }
      if (reparse == 0)
        return true;
      if (reparse == 2)
        break;

      if (vfs->Loaded(path_to_index))
        return true;
      LOG_S(INFO) << "load cache for " << path_to_index;
      auto dependencies = prev->dependencies;
      IndexUpdate update = IndexUpdate::CreateDelta(nullptr, prev.get());
      on_indexed->PushBack(std::move(update),
        request.mode != IndexMode::NonInteractive);
      {
        std::lock_guard lock1(vfs->mutex);
        vfs->state[path_to_index].loaded = true;
      }
      lock.unlock();

      for (const auto &dep : dependencies) {
        std::string path = dep.first.val().str();
        if (!vfs->Stamp(path, dep.second, 1))
          continue;
        std::lock_guard lock1(GetFileMutex(path));
        prev = RawCacheLoad(path);
        if (!prev)
          continue;
        {
          std::lock_guard lock2(vfs->mutex);
          VFS::State &st = vfs->state[path];
          if (st.loaded)
            continue;
          st.loaded = true;
          st.timestamp = prev->mtime;
        }
        IndexUpdate update = IndexUpdate::CreateDelta(nullptr, prev.get());
        on_indexed->PushBack(std::move(update),
          request.mode != IndexMode::NonInteractive);
        if (entry.id >= 0) {
          std::shared_lock lock2(project->mtx);
          project->root2folder[entry.root].path2entry_index[path] = entry.id;
        }
      }
      return true;
    } while (0);

  if (loud) {
    std::string line;
    if (LOG_V_ENABLED(1)) {
      line = "\n ";
      for (auto &arg : entry.args)
        (line += ' ') += arg;
    }
    LOG_S(INFO) << (deleted ? "delete " : "parse ") << path_to_index << line;
  }

  std::vector<std::unique_ptr<IndexFile>> indexes;
  if (deleted) {
    indexes.push_back(std::make_unique<IndexFile>(request.path, ""));
    if (request.path != path_to_index)
      indexes.push_back(std::make_unique<IndexFile>(path_to_index, ""));
  } else {
    std::vector<std::pair<std::string, std::string>> remapped;
    if (g_config->index.onChange) {
      std::string content = wfiles->GetContent(path_to_index);
      if (content.size())
        remapped.emplace_back(path_to_index, content);
    }
    bool ok;
    indexes = idx::Index(completion, wfiles, vfs, entry.directory,
                         path_to_index, entry.args, remapped, ok);

    if (!ok) {
      if (request.id.Valid()) {
        ResponseError err;
        err.code = ErrorCode::InternalError;
        err.message = "failed to index " + path_to_index;
        pipeline::ReplyError(request.id, err);
      }
      return true;
    }
  }

  for (std::unique_ptr<IndexFile> &curr : indexes) {
    std::string path = curr->path;
    if (!matcher.Matches(path)) {
      LOG_IF_S(INFO, loud) << "skip index for " << path;
      continue;
    }

    if (!deleted)
      LOG_IF_S(INFO, loud) << "store index for " << path
                           << " (delta: " << !!prev << ")";
    {
      std::lock_guard lock(GetFileMutex(path));
      if (vfs->Loaded(path))
        prev = RawCacheLoad(path);
      else
        prev.reset();
      if (g_config->cacheDirectory.empty()) {
        std::lock_guard lock(g_index_mutex);
        auto it = g_index.insert_or_assign(
          path, InMemoryIndexFile{curr->file_contents, *curr});
        std::string().swap(it.first->second.index.file_contents);
      } else {
        std::string cache_path = GetCachePath(path);
        if (deleted) {
          (void)sys::fs::remove(cache_path);
          (void)sys::fs::remove(AppendSerializationFormat(cache_path));
        } else {
          WriteToFile(cache_path, curr->file_contents);
          WriteToFile(AppendSerializationFormat(cache_path),
                      Serialize(g_config->cacheFormat, *curr));
        }
      }
      on_indexed->PushBack(IndexUpdate::CreateDelta(prev.get(), curr.get()),
                           request.mode != IndexMode::NonInteractive);
      {
        std::lock_guard lock1(vfs->mutex);
        vfs->state[path].loaded = true;
      }
      if (entry.id >= 0) {
        std::shared_lock lock(project->mtx);
        auto &folder = project->root2folder[entry.root];
        for (auto &dep : curr->dependencies)
          folder.path2entry_index[dep.first.val().str()] = entry.id;
      }
    }
  }

  return true;
}

void Quit(SemaManager &manager) {
  quit.store(true, std::memory_order_relaxed);
  manager.Quit();

  { std::lock_guard lock(index_request->mutex_); }
  indexer_waiter->cv.notify_all();
  { std::lock_guard lock(for_stdout->mutex_); }
  stdout_waiter->cv.notify_one();
  std::unique_lock lock(thread_mtx);
  no_active_threads.wait(lock, [] { return !active_threads; });
}

} // namespace

void ThreadEnter() {
  std::lock_guard lock(thread_mtx);
  active_threads++;
}

void ThreadLeave() {
  std::lock_guard lock(thread_mtx);
  if (!--active_threads)
    no_active_threads.notify_one();
}

void Init() {
  main_waiter = new MultiQueueWaiter;
  on_request = new ThreadedQueue<InMessage>(main_waiter);
  on_indexed = new ThreadedQueue<IndexUpdate>(main_waiter);

  indexer_waiter = new MultiQueueWaiter;
  index_request = new ThreadedQueue<IndexRequest>(indexer_waiter);

  stdout_waiter = new MultiQueueWaiter;
  for_stdout = new ThreadedQueue<std::string>(stdout_waiter);
}

void Indexer_Main(SemaManager *manager, VFS *vfs, Project *project,
                  WorkingFiles *wfiles) {
  GroupMatch matcher(g_config->index.whitelist, g_config->index.blacklist);
  while (true)
    if (!Indexer_Parse(manager, wfiles, project, vfs, matcher))
      if (indexer_waiter->Wait(quit, index_request))
        break;
}

void Main_OnIndexed(DB *db, WorkingFiles *wfiles, IndexUpdate *update) {
  if (update->refresh) {
    LOG_S(INFO)
        << "loaded project. Refresh semantic highlight for all working file.";
    std::lock_guard lock(wfiles->mutex);
    for (auto &[f, wf] : wfiles->files) {
      std::string path = LowerPathIfInsensitive(f);
      if (db->name2file_id.find(path) == db->name2file_id.end())
        continue;
      QueryFile &file = db->files[db->name2file_id[path]];
      EmitSemanticHighlight(db, wf.get(), file);
    }
    return;
  }

  db->ApplyIndexUpdate(update);

  // Update indexed content, skipped ranges, and semantic highlighting.
  if (update->files_def_update) {
    auto &def_u = *update->files_def_update;
    if (WorkingFile *wfile = wfiles->GetFile(def_u.first.path)) {
      // FIXME With index.onChange: true, use buffer_content only for
      // request.path
      wfile->SetIndexContent(g_config->index.onChange ? wfile->buffer_content
                                                      : def_u.second);
      QueryFile &file = db->files[update->file_id];
      EmitSkippedRanges(wfile, file);
      EmitSemanticHighlight(db, wfile, file);
    }
  }
}

void LaunchStdin() {
  ThreadEnter();
  std::thread([]() {
    set_thread_name("stdin");
    std::string str;
    const std::string_view kContentLength("Content-Length: ");
    while (true) {
      int len = 0;
      str.clear();
      while (true) {
        int c = getchar();
        if (c == EOF)
          return;
        if (c == '\n') {
          if (str.empty())
            break;
          if (!str.compare(0, kContentLength.size(), kContentLength))
            len = atoi(str.c_str() + kContentLength.size());
          str.clear();
        } else if (c != '\r') {
          str += c;
        }
      }

      str.resize(len);
      for (int i = 0; i < len; ++i) {
        int c = getchar();
        if (c == EOF)
          return;
        str[i] = c;
      }

      auto message = std::make_unique<char[]>(len);
      std::copy(str.begin(), str.end(), message.get());
      auto document = std::make_unique<rapidjson::Document>();
      document->Parse(message.get(), len);
      assert(!document->HasParseError());

      JsonReader reader{document.get()};
      if (!reader.m->HasMember("jsonrpc") ||
          std::string((*reader.m)["jsonrpc"].GetString()) != "2.0")
        break;
      RequestId id;
      std::string method;
      ReflectMember(reader, "id", id);
      ReflectMember(reader, "method", method);
      if (method.empty())
        continue;
      bool should_exit = method == "exit";
      on_request->PushBack(
          {id, std::move(method), std::move(message), std::move(document)});

      if (should_exit)
        break;
    }
    ThreadLeave();
  }).detach();
}

void LaunchStdout() {
  ThreadEnter();
  std::thread([]() {
    set_thread_name("stdout");

    while (true) {
      std::vector<std::string> messages = for_stdout->DequeueAll();
      for (auto &s : messages) {
        llvm::outs() << "Content-Length: " << s.size() << "\r\n\r\n" << s;
        llvm::outs().flush();
      }
      if (stdout_waiter->Wait(quit, for_stdout))
        break;
    }
    ThreadLeave();
  }).detach();
}

void MainLoop() {
  Project project;
  WorkingFiles wfiles;
  VFS vfs;

  SemaManager manager(
      &project, &wfiles,
      [&](std::string path, std::vector<Diagnostic> diagnostics) {
        PublishDiagnosticParam params;
        params.uri = DocumentUri::FromPath(path);
        params.diagnostics = diagnostics;
        Notify("textDocument/publishDiagnostics", params);
      },
      [](RequestId id) {
        if (id.Valid()) {
          ResponseError err;
          err.code = ErrorCode::InternalError;
          err.message = "drop older completion request";
          ReplyError(id, err);
        }
      });

  IncludeComplete include_complete(&project);
  DB db;

  // Setup shared references.
  MessageHandler handler;
  handler.db = &db;
  handler.project = &project;
  handler.vfs = &vfs;
  handler.wfiles = &wfiles;
  handler.manager = &manager;
  handler.include_complete = &include_complete;

  bool has_indexed = false;
  while (true) {
    std::vector<InMessage> messages = on_request->DequeueAll();
    bool did_work = messages.size();
    for (InMessage &message : messages)
      handler.Run(message);

    bool indexed = false;
    for (int i = 20; i--;) {
      std::optional<IndexUpdate> update = on_indexed->TryPopFront();
      if (!update)
        break;
      did_work = true;
      indexed = true;
      Main_OnIndexed(&db, &wfiles, &*update);
    }

    if (did_work) {
      has_indexed |= indexed;
      if (quit.load(std::memory_order_relaxed))
        break;
    } else {
      if (has_indexed) {
        FreeUnusedMemory();
        has_indexed = false;
      }
      main_waiter->Wait(quit, on_indexed, on_request);
    }
  }

  Quit(manager);
}

void Standalone(const std::string &root) {
  Project project;
  WorkingFiles wfiles;
  VFS vfs;
  SemaManager manager(
      nullptr, nullptr, [&](std::string, std::vector<Diagnostic>) {},
      [](RequestId id) {});
  IncludeComplete complete(&project);

  MessageHandler handler;
  handler.project = &project;
  handler.wfiles = &wfiles;
  handler.vfs = &vfs;
  handler.manager = &manager;
  handler.include_complete = &complete;

  StandaloneInitialize(handler, root);
  bool tty = sys::Process::StandardOutIsDisplayed();

  if (tty) {
    int entries = 0;
    for (auto &[_, folder] : project.root2folder)
      entries += folder.entries.size();
    printf("entries: %5d\n", entries);
  }
  while (1) {
    (void)on_indexed->DequeueAll();
    int pending = pending_index_requests;
    if (tty) {
      printf("\rpending: %5d", pending);
      fflush(stdout);
    }
    if (!pending)
      break;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  if (tty)
    puts("");
  Quit(manager);
}

void Index(const std::string &path, const std::vector<const char *> &args,
           IndexMode mode, bool must_exist, RequestId id) {
  pending_index_requests++;
  index_request->PushBack({path, args, mode, must_exist, id},
                          mode != IndexMode::NonInteractive);
}

std::optional<std::string> LoadIndexedContent(const std::string &path) {
  if (g_config->cacheDirectory.empty()) {
    std::shared_lock lock(g_index_mutex);
    auto it = g_index.find(path);
    if (it == g_index.end())
      return {};
    return it->second.content;
  }
  return ReadContent(GetCachePath(path));
}

void NotifyOrRequest(const char *method, bool request,
                     const std::function<void(JsonWriter &)> &fn) {
  rapidjson::StringBuffer output;
  rapidjson::Writer<rapidjson::StringBuffer> w(output);
  w.StartObject();
  w.Key("jsonrpc");
  w.String("2.0");
  w.Key("method");
  w.String(method);
  if (request) {
    w.Key("id");
    w.Int64(request_id.fetch_add(1, std::memory_order_relaxed));
  }
  w.Key("params");
  JsonWriter writer(&w);
  fn(writer);
  w.EndObject();
  for_stdout->PushBack(output.GetString());
}

static void Reply(RequestId id, const char *key,
                  const std::function<void(JsonWriter &)> &fn) {
  rapidjson::StringBuffer output;
  rapidjson::Writer<rapidjson::StringBuffer> w(output);
  w.StartObject();
  w.Key("jsonrpc");
  w.String("2.0");
  w.Key("id");
  switch (id.type) {
  case RequestId::kNone:
    w.Null();
    break;
  case RequestId::kInt:
    w.Int(id.value);
    break;
  case RequestId::kString:
    auto s = std::to_string(id.value);
    w.String(s.c_str(), s.length());
    break;
  }
  w.Key(key);
  JsonWriter writer(&w);
  fn(writer);
  w.EndObject();
  for_stdout->PushBack(output.GetString());
}

void Reply(RequestId id, const std::function<void(JsonWriter &)> &fn) {
  Reply(id, "result", fn);
}

void ReplyError(RequestId id, const std::function<void(JsonWriter &)> &fn) {
  Reply(id, "error", fn);
}
} // namespace pipeline
} // namespace ccls
