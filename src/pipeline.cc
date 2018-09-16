// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "pipeline.hh"

#include "clang_complete.hh"
#include "config.h"
#include "include_complete.h"
#include "log.hh"
#include "lsp.h"
#include "message_handler.h"
#include "pipeline.hh"
#include "platform.h"
#include "project.h"
#include "query_utils.h"

#include <llvm/ADT/Twine.h>
#include <llvm/Support/Threading.h>
#include <llvm/Support/Timer.h>
using namespace llvm;

#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <thread>
#ifndef _WIN32
#include <unistd.h>
#endif

void DiagnosticsPublisher::Init() {
  frequencyMs_ = g_config->diagnostics.frequencyMs;
  match_ = std::make_unique<GroupMatch>(g_config->diagnostics.whitelist,
                                        g_config->diagnostics.blacklist);
}

void DiagnosticsPublisher::Publish(WorkingFiles *working_files,
                                   std::string path,
                                   std::vector<lsDiagnostic> diagnostics) {
  bool good = true;
  // Cache diagnostics so we can show fixits.
  working_files->DoActionOnFile(path, [&](WorkingFile *working_file) {
    if (working_file) {
      good = working_file->diagnostics_.empty();
      working_file->diagnostics_ = diagnostics;
    }
  });

  int64_t now =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::high_resolution_clock::now().time_since_epoch())
          .count();
  if (frequencyMs_ >= 0 &&
      (nextPublish_ <= now || (!good && diagnostics.empty())) &&
      match_->IsMatch(path)) {
    nextPublish_ = now + frequencyMs_;

    Out_TextDocumentPublishDiagnostics out;
    out.params.uri = lsDocumentUri::FromPath(path);
    out.params.diagnostics = diagnostics;
    ccls::pipeline::WriteStdout(kMethodType_TextDocumentPublishDiagnostics,
                                out);
  }
}

namespace ccls::pipeline {
namespace {

struct Index_Request {
  std::string path;
  std::vector<std::string> args;
  IndexMode mode;
  lsRequestId id;
};

struct Stdout_Request {
  MethodType method;
  std::string content;
};

MultiQueueWaiter *main_waiter;
MultiQueueWaiter *indexer_waiter;
MultiQueueWaiter *stdout_waiter;
ThreadedQueue<std::unique_ptr<InMessage>> *on_request;
ThreadedQueue<Index_Request> *index_request;
ThreadedQueue<IndexUpdate> *on_indexed;
ThreadedQueue<Stdout_Request> *for_stdout;

struct InMemoryIndexFile {
  std::string content;
  IndexFile index;
};
std::shared_mutex g_index_mutex;
std::unordered_map<std::string, InMemoryIndexFile> g_index;

bool CacheInvalid(VFS *vfs, IndexFile *prev, const std::string &path,
                  const std::vector<std::string> &args,
                  const std::optional<std::string> &from) {
  {
    std::lock_guard<std::mutex> lock(vfs->mutex);
    if (prev->mtime < vfs->state[path].timestamp) {
      LOG_S(INFO) << "timestamp changed for " << path
                  << (from ? " (via " + *from + ")" : std::string());
      return true;
    }
  }

  if (prev->args != args) {
    LOG_S(INFO) << "args changed for " << path
                << (from ? " (via " + *from + ")" : std::string());
    return true;
  }

  return false;
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
  std::string cache_file;
  auto len = g_config->projectRoot.size();
  if (StartsWith(source_file, g_config->projectRoot)) {
    cache_file = EscapeFileName(g_config->projectRoot.substr(0, len - 1)) + '/' +
                 EscapeFileName(source_file.substr(len));
  } else {
    cache_file = '@' +
                 EscapeFileName(g_config->projectRoot.substr(0, len - 1)) + '/' +
                 EscapeFileName(source_file);
  }

  return g_config->cacheDirectory + cache_file;
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

bool Indexer_Parse(CompletionManager *completion, WorkingFiles *wfiles,
                   Project *project, VFS *vfs, const GroupMatch &matcher) {
  std::optional<Index_Request> opt_request = index_request->TryPopFront();
  if (!opt_request)
    return false;
  auto &request = *opt_request;
  bool loud = request.mode != IndexMode::OnChange;

  // Dummy one to trigger refresh semantic highlight.
  if (request.path.empty()) {
    IndexUpdate dummy;
    dummy.refresh = true;
    on_indexed->PushBack(std::move(dummy), false);
    return false;
  }

  if (std::string reason; !matcher.IsMatch(request.path, &reason)) {
    LOG_IF_S(INFO, loud) << "skip " << request.path << " for " << reason;
    return false;
  }

  Project::Entry entry = project->FindCompilationEntryForFile(request.path);
  if (request.args.size())
    entry.args = request.args;
  std::string path_to_index = entry.filename;
  std::unique_ptr<IndexFile> prev;

  std::optional<int64_t> write_time = LastWriteTime(path_to_index);
  if (!write_time)
    return true;
  int reparse = vfs->Stamp(path_to_index, *write_time);
  if (request.path != path_to_index) {
    std::optional<int64_t> mtime1 = LastWriteTime(request.path);
    if (!mtime1)
      return true;
    if (vfs->Stamp(request.path, *mtime1))
      reparse = 2;
  }
  if (g_config->index.onChange)
    reparse = 2;
  if (!vfs->Mark(path_to_index, g_thread_id, 1) && !reparse)
    return true;

  prev = RawCacheLoad(path_to_index);
  if (!prev)
    reparse = 2;
  else {
    if (CacheInvalid(vfs, prev.get(), path_to_index, entry.args, std::nullopt))
      reparse = 2;
    int reparseForDep = g_config->index.reparseForDependency;
    if (reparseForDep > 1 || (reparseForDep == 1 && !Project::loaded))
      for (const auto &dep : prev->dependencies) {
        if (auto write_time1 = LastWriteTime(dep.first().str())) {
          if (dep.second < *write_time1) {
            reparse = 2;
            std::lock_guard<std::mutex> lock(vfs->mutex);
            vfs->state[dep.first().str()].stage = 0;
          }
        } else
          reparse = 2;
      }
  }

  // Grab the ownership
  if (reparse) {
    std::lock_guard<std::mutex> lock(vfs->mutex);
    vfs->state[path_to_index].owner = g_thread_id;
    vfs->state[path_to_index].stage = 0;
  }

  if (reparse < 2) {
    LOG_S(INFO) << "load cache for " << path_to_index;
    auto dependencies = prev->dependencies;
    if (reparse) {
      IndexUpdate update = IndexUpdate::CreateDelta(nullptr, prev.get());
      on_indexed->PushBack(std::move(update),
                           request.mode != IndexMode::NonInteractive);
      std::lock_guard lock(vfs->mutex);
      vfs->state[path_to_index].loaded = true;
    }
    for (const auto &dep : dependencies) {
      std::string path = dep.first().str();
      if (vfs->Mark(path, 0, 2) && (prev = RawCacheLoad(path))) {
        IndexUpdate update = IndexUpdate::CreateDelta(nullptr, prev.get());
        on_indexed->PushBack(std::move(update),
                             request.mode != IndexMode::NonInteractive);
        {
          std::lock_guard lock(vfs->mutex);
          vfs->state[path].loaded = true;
        }
        if (entry.id >= 0) {
          std::lock_guard lock(project->mutex_);
          project->path_to_entry_index[path] = entry.id;
        }
      }
    }
    return true;
  }

  LOG_IF_S(INFO, loud) << "parse " << path_to_index;

  std::vector<std::pair<std::string, std::string>> remapped;
  if (g_config->index.onChange) {
    std::string content = wfiles->GetContent(path_to_index);
    if (content.size())
      remapped.emplace_back(path_to_index, content);
  }
  auto indexes = idx::Index(completion, wfiles, vfs, entry.directory,
                            path_to_index, entry.args, remapped);

  if (indexes.empty()) {
    if (g_config->index.enabled && request.id.Valid()) {
      Out_Error out;
      out.id = request.id;
      out.error.code = lsErrorCodes::InternalError;
      out.error.message = "Failed to index " + path_to_index;
      pipeline::WriteStdout(kMethodType_Unknown, out);
    }
    vfs->Reset(path_to_index);
    return true;
  }

  for (std::unique_ptr<IndexFile> &curr : indexes) {
    std::string path = curr->path;
    bool do_update = path == path_to_index || path == request.path, loaded;
    {
      std::lock_guard<std::mutex> lock(vfs->mutex);
      VFS::State &st = vfs->state[path];
      if (st.timestamp < curr->mtime) {
        st.timestamp = curr->mtime;
        do_update = true;
      }
      loaded = st.loaded;
      st.loaded = true;
    }
    if (std::string reason; !matcher.IsMatch(path, &reason)) {
      LOG_IF_S(INFO, loud) << "skip emitting and storing index of " << path << " for "
                           << reason;
      do_update = false;
    }
    if (!do_update) {
      vfs->Reset(path);
      continue;
    }

    prev.reset();
    if (loaded)
      prev = RawCacheLoad(path);

    // Store current index.
    LOG_IF_S(INFO, loud) << "store index for " << path << " (delta: " << !!prev
                         << ")";
    if (g_config->cacheDirectory.empty()) {
      std::lock_guard lock(g_index_mutex);
      auto it = g_index.insert_or_assign(
        path, InMemoryIndexFile{curr->file_contents, *curr});
      std::string().swap(it.first->second.index.file_contents);
    } else {
      std::string cache_path = GetCachePath(path);
      WriteToFile(cache_path, curr->file_contents);
      WriteToFile(AppendSerializationFormat(cache_path),
        Serialize(g_config->cacheFormat, *curr));
    }

    vfs->Reset(path);
    if (entry.id >= 0) {
      std::lock_guard<std::mutex> lock(project->mutex_);
      for (auto &dep : curr->dependencies)
        project->path_to_entry_index[dep.first()] = entry.id;
    }

    // Build delta update.
    IndexUpdate update = IndexUpdate::CreateDelta(prev.get(), curr.get());

    on_indexed->PushBack(std::move(update),
                         request.mode != IndexMode::NonInteractive);
  }

  return true;
}

} // namespace

void Init() {
  main_waiter = new MultiQueueWaiter;
  on_request = new ThreadedQueue<std::unique_ptr<InMessage>>(main_waiter);
  on_indexed = new ThreadedQueue<IndexUpdate>(main_waiter);

  indexer_waiter = new MultiQueueWaiter;
  index_request = new ThreadedQueue<Index_Request>(indexer_waiter);

  stdout_waiter = new MultiQueueWaiter;
  for_stdout = new ThreadedQueue<Stdout_Request>(stdout_waiter);
}

void Indexer_Main(CompletionManager *completion, VFS *vfs, Project *project,
                  WorkingFiles *wfiles) {
  GroupMatch matcher(g_config->index.whitelist, g_config->index.blacklist);
  while (true)
    if (!Indexer_Parse(completion, wfiles, project, vfs, matcher))
      indexer_waiter->Wait(index_request);
}

void Main_OnIndexed(DB *db, SemanticHighlightSymbolCache *semantic_cache,
                    WorkingFiles *working_files, IndexUpdate *update) {
  if (update->refresh) {
    Project::loaded = true;
    LOG_S(INFO)
        << "loaded project. Refresh semantic highlight for all working file.";
    std::lock_guard<std::mutex> lock(working_files->files_mutex);
    for (auto &f : working_files->files) {
      std::string filename = LowerPathIfInsensitive(f->filename);
      if (db->name2file_id.find(filename) == db->name2file_id.end())
        continue;
      QueryFile *file = &db->files[db->name2file_id[filename]];
      EmitSemanticHighlighting(db, semantic_cache, f.get(), file);
    }
    return;
  }

  static Timer timer("apply", "apply index");
  timer.startTimer();
  db->ApplyIndexUpdate(update);
  timer.stopTimer();

  // Update indexed content, skipped ranges, and semantic highlighting.
  if (update->files_def_update) {
    auto &def_u = *update->files_def_update;
    if (WorkingFile *wfile =
            working_files->GetFileByFilename(def_u.first.path)) {
      // FIXME With index.onChange: true, use buffer_content only for
      // request.path
      wfile->SetIndexContent(g_config->index.onChange ? wfile->buffer_content
                                                      : def_u.second);
      EmitSkippedRanges(wfile, def_u.first.skipped_ranges);
      EmitSemanticHighlighting(db, semantic_cache, wfile,
                               &db->files[update->file_id]);
    }
  }
}

void LaunchStdin() {
  std::thread([]() {
    set_thread_name("stdin");
    while (true) {
      std::unique_ptr<InMessage> message;
      std::optional<std::string> err =
          MessageRegistry::instance()->ReadMessageFromStdin(&message);

      // Message parsing can fail if we don't recognize the method.
      if (err) {
        // The message may be partially deserialized.
        // Emit an error ResponseMessage if |id| is available.
        if (message) {
          lsRequestId id = message->GetRequestId();
          if (id.Valid()) {
            Out_Error out;
            out.id = id;
            out.error.code = lsErrorCodes::InvalidParams;
            out.error.message = std::move(*err);
            WriteStdout(kMethodType_Unknown, out);
          }
        }
        continue;
      }

      // Cache |method_id| so we can access it after moving |message|.
      MethodType method_type = message->GetMethodType();

      on_request->PushBack(std::move(message));

      // If the message was to exit then querydb will take care of the actual
      // exit. Stop reading from stdin since it might be detached.
      if (method_type == kMethodType_Exit)
        break;
    }
  })
      .detach();
}

void LaunchStdout() {
  std::thread([=]() {
    set_thread_name("stdout");

    while (true) {
      std::vector<Stdout_Request> messages = for_stdout->DequeueAll();
      if (messages.empty()) {
        stdout_waiter->Wait(for_stdout);
        continue;
      }

      for (auto &message : messages) {
#ifdef _WIN32
        fwrite(message.content.c_str(), message.content.size(), 1, stdout);
        fflush(stdout);
#else
        write(1, message.content.c_str(), message.content.size());
#endif
      }
    }
  })
      .detach();
}

void MainLoop() {
  Project project;
  SemanticHighlightSymbolCache semantic_cache;
  WorkingFiles working_files;
  VFS vfs;
  DiagnosticsPublisher diag_pub;

  CompletionManager clang_complete(
      &project, &working_files,
      [&](std::string path, std::vector<lsDiagnostic> diagnostics) {
        diag_pub.Publish(&working_files, path, diagnostics);
      },
      [](lsRequestId id) {
        if (id.Valid()) {
          Out_Error out;
          out.id = id;
          out.error.code = lsErrorCodes::InternalError;
          out.error.message = "Dropping completion request; a newer request "
                              "has come in that will be serviced instead.";
          pipeline::WriteStdout(kMethodType_Unknown, out);
        }
      });

  IncludeComplete include_complete(&project);
  DB db;

  // Setup shared references.
  for (MessageHandler *handler : *MessageHandler::message_handlers) {
    handler->db = &db;
    handler->waiter = indexer_waiter;
    handler->project = &project;
    handler->diag_pub = &diag_pub;
    handler->vfs = &vfs;
    handler->semantic_cache = &semantic_cache;
    handler->working_files = &working_files;
    handler->clang_complete = &clang_complete;
    handler->include_complete = &include_complete;
  }

  while (true) {
    std::vector<std::unique_ptr<InMessage>> messages = on_request->DequeueAll();
    bool did_work = messages.size();
    for (auto &message : messages) {
      // TODO: Consider using std::unordered_map to lookup the handler
      for (MessageHandler *handler : *MessageHandler::message_handlers) {
        if (handler->GetMethodType() == message->GetMethodType()) {
          handler->Run(std::move(message));
          break;
        }
      }

      if (message)
        LOG_S(ERROR) << "No handler for " << message->GetMethodType();
    }

    for (int i = 80; i--;) {
      std::optional<IndexUpdate> update = on_indexed->TryPopFront();
      if (!update)
        break;
      did_work = true;
      Main_OnIndexed(&db, &semantic_cache, &working_files, &*update);
    }

    if (!did_work) {
      FreeUnusedMemory();
      main_waiter->Wait(on_indexed, on_request);
    }
  }
}

void Index(const std::string &path, const std::vector<std::string> &args,
           IndexMode mode, lsRequestId id) {
  index_request->PushBack({path, args, mode, id}, mode != IndexMode::NonInteractive);
}

std::optional<int64_t> LastWriteTime(const std::string &path) {
  sys::fs::file_status Status;
  if (sys::fs::status(path, Status))
    return {};
  return sys::toTimeT(Status.getLastModificationTime());
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

void WriteStdout(MethodType method, lsBaseOutMessage &response) {
  std::ostringstream sstream;
  response.Write(sstream);

  Stdout_Request out;
  out.content = sstream.str();
  out.method = method;
  for_stdout->PushBack(std::move(out));
}

} // namespace ccls::pipeline
