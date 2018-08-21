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

#include "clang_complete.h"
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
  bool is_interactive;
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

bool CacheInvalid(VFS *vfs, IndexFile *prev, const std::string &path,
                  const std::vector<std::string> &args,
                  const std::optional<std::string> &from) {
  {
    std::lock_guard<std::mutex> lock(vfs->mutex);
    if (prev->last_write_time < vfs->state[path].timestamp) {
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
  size_t len = g_config->projectRoot.size();
  if (StartsWith(source_file, g_config->projectRoot)) {
    cache_file = EscapeFileName(g_config->projectRoot) +
                 EscapeFileName(source_file.substr(len));
  } else {
    cache_file = '@' + EscapeFileName(g_config->projectRoot) +
                 EscapeFileName(source_file);
  }

  return g_config->cacheDirectory + cache_file;
}

std::unique_ptr<IndexFile> RawCacheLoad(const std::string &path) {
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

bool Indexer_Parse(DiagnosticsPublisher *diag_pub, WorkingFiles *working_files,
                   Project *project, VFS *vfs) {
  std::optional<Index_Request> opt_request = index_request->TryPopFront();
  if (!opt_request)
    return false;
  auto &request = *opt_request;

  // Dummy one to trigger refresh semantic highlight.
  if (request.path.empty()) {
    IndexUpdate dummy;
    dummy.refresh = true;
    on_indexed->PushBack(std::move(dummy), false);
    return false;
  }

  Project::Entry entry;
  {
    std::lock_guard<std::mutex> lock(project->mutex_);
    auto it = project->absolute_path_to_entry_index_.find(request.path);
    if (it != project->absolute_path_to_entry_index_.end())
      entry = project->entries[it->second];
    else {
      entry.filename = request.path;
      entry.args = request.args;
    }
  }
  std::string path_to_index = entry.filename;
  std::unique_ptr<IndexFile> prev;

  // Try to load the file from cache.
  std::optional<int64_t> write_time = LastWriteTime(path_to_index);
  if (!write_time)
    return true;
  int reparse = vfs->Stamp(path_to_index, *write_time);
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
      on_indexed->PushBack(std::move(update), request.is_interactive);
    }
    for (const auto &dep : dependencies)
      if (vfs->Mark(dep.first().str(), 0, 2) &&
          (prev = RawCacheLoad(dep.first().str()))) {
        IndexUpdate update = IndexUpdate::CreateDelta(nullptr, prev.get());
        on_indexed->PushBack(std::move(update), request.is_interactive);
      }
    return true;
  }

  LOG_S(INFO) << "parse " << path_to_index;

  auto indexes =
      idx::Index(vfs, entry.directory, path_to_index, entry.args, {});

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
    // Only emit diagnostics for non-interactive sessions, which makes it easier
    // to identify indexing problems. For interactive sessions, diagnostics are
    // handled by code completion.
    if (!request.is_interactive)
      diag_pub->Publish(working_files, curr->path, curr->diagnostics_);

    std::string path = curr->path;
    if (!(vfs->Stamp(path, curr->last_write_time) || path == path_to_index))
      continue;
    LOG_S(INFO) << "emit index for " << path;
    prev = RawCacheLoad(path);

    // Write current index to disk if requested.
    LOG_S(INFO) << "store index for " << path;
    {
      static Timer timer("write", "store index");
      timer.startTimer();
      std::string cache_path = GetCachePath(path);
      WriteToFile(cache_path, curr->file_contents);
      WriteToFile(AppendSerializationFormat(cache_path),
                  Serialize(g_config->cacheFormat, *curr));
      timer.stopTimer();
    }

    vfs->Reset(path);
    if (entry.id >= 0) {
      std::lock_guard<std::mutex> lock(project->mutex_);
      for (auto &dep : curr->dependencies)
        project->absolute_path_to_entry_index_[dep.first()] = entry.id;
    }

    // Build delta update.
    IndexUpdate update = IndexUpdate::CreateDelta(prev.get(), curr.get());
    LOG_S(INFO) << "built index for " << path << " (is_delta=" << !!prev << ")";

    on_indexed->PushBack(std::move(update), request.is_interactive);
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

void Indexer_Main(DiagnosticsPublisher *diag_pub, VFS *vfs, Project *project,
                  WorkingFiles *working_files) {
  while (true)
    if (!Indexer_Parse(diag_pub, working_files, project, vfs))
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
    LOG_S(INFO) << "apply index for " << def_u.first.path;
    if (WorkingFile *working_file =
            working_files->GetFileByFilename(def_u.first.path)) {
      working_file->SetIndexContent(def_u.second);
      EmitSkippedRanges(working_file, def_u.first.skipped_ranges);
      EmitSemanticHighlighting(db, semantic_cache, working_file,
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

  ClangCompleteManager clang_complete(
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
  auto global_code_complete_cache = std::make_unique<CodeCompleteCache>();
  auto non_global_code_complete_cache = std::make_unique<CodeCompleteCache>();
  auto signature_cache = std::make_unique<CodeCompleteCache>();
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
    handler->global_code_complete_cache = global_code_complete_cache.get();
    handler->non_global_code_complete_cache =
        non_global_code_complete_cache.get();
    handler->signature_cache = signature_cache.get();
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
           bool interactive, lsRequestId id) {
  index_request->PushBack({path, args, interactive, id}, interactive);
}

std::optional<std::string> LoadCachedFileContents(const std::string &path) {
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
