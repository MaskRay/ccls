#include "pipeline.hh"

#include "clang_complete.h"
#include "config.h"
#include "diagnostics_engine.h"
#include "include_complete.h"
#include "log.hh"
#include "lsp.h"
#include "message_handler.h"
#include "platform.h"
#include "project.h"
#include "query_utils.h"
#include "pipeline.hh"
#include "timer.h"

#include <llvm/ADT/Twine.h>
#include <llvm/Support/Threading.h>
using namespace llvm;

#include <thread>

struct Index_Request {
  std::string path;
  std::vector<std::string> args;
  bool is_interactive;
  lsRequestId id;
};

struct Index_OnIndexed {
  IndexUpdate update;
  PerformanceImportFile perf;
};

struct Stdout_Request {
  MethodType method;
  std::string content;
};

namespace ccls::pipeline {
namespace {

MultiQueueWaiter* main_waiter;
MultiQueueWaiter* indexer_waiter;
MultiQueueWaiter* stdout_waiter;
ThreadedQueue<std::unique_ptr<InMessage>>* on_request;
ThreadedQueue<Index_Request>* index_request;
ThreadedQueue<Index_OnIndexed>* on_indexed;
ThreadedQueue<Stdout_Request>* for_stdout;

// Checks if |path| needs to be reparsed. This will modify cached state
// such that calling this function twice with the same path may return true
// the first time but will return false the second.
//
// |from|: The file which generated the parse request for this file.
bool FileNeedsParse(int64_t write_time,
                    VFS* vfs,
                    bool is_interactive,
                    IndexFile* opt_previous_index,
                    const std::string& path,
                    const std::vector<std::string>& args,
                    const std::optional<std::string>& from) {
  {
    std::lock_guard<std::mutex> lock(vfs->mutex);
    if (vfs->state[path].timestamp < write_time) {
      LOG_S(INFO) << "timestamp changed for " << path
                  << (from ? " (via " + *from + ")" : std::string());
      return true;
    }
  }

  // Command-line arguments changed.
  auto is_file = [](const std::string& arg) {
    return EndsWithAny(arg, {".h", ".c", ".cc", ".cpp", ".hpp", ".m", ".mm"});
  };
  if (opt_previous_index) {
    auto& prev_args = opt_previous_index->args;
    bool same = prev_args.size() == args.size();
    for (size_t i = 0; i < args.size() && same; ++i) {
      same = prev_args[i] == args[i] ||
             (is_file(prev_args[i]) && is_file(args[i]));
    }
    if (!same) {
      LOG_S(INFO) << "args changed for " << path << (from ? " (via " + *from + ")" : std::string());
      return true;
    }
  }

  return false;
};

std::string AppendSerializationFormat(const std::string& base) {
  switch (g_config->cacheFormat) {
    case SerializeFormat::Binary:
      return base + ".blob";
    case SerializeFormat::Json:
      return base + ".json";
  }
}

std::string GetCachePath(const std::string& source_file) {
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

std::unique_ptr<IndexFile> RawCacheLoad(
    const std::string& path) {
  std::string cache_path = GetCachePath(path);
  std::optional<std::string> file_content = ReadContent(cache_path);
  std::optional<std::string> serialized_indexed_content =
      ReadContent(AppendSerializationFormat(cache_path));
  if (!file_content || !serialized_indexed_content)
    return nullptr;

  return Deserialize(g_config->cacheFormat, path, *serialized_indexed_content,
                     *file_content, IndexFile::kMajorVersion);
}

bool Indexer_Parse(DiagnosticsEngine* diag_engine,
                   WorkingFiles* working_files,
                   Project* project,
                   VFS* vfs,
                   ClangIndexer* indexer) {
  std::optional<Index_Request> opt_request = index_request->TryPopFront();
  if (!opt_request)
    return false;
  auto& request = *opt_request;

  // Dummy one to trigger refresh semantic highlight.
  if (request.path.empty()) {
    IndexUpdate dummy;
    dummy.refresh = true;
    on_indexed->PushBack({std::move(dummy), PerformanceImportFile()}, false);
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
  // FIXME Don't drop
  if (!vfs->Mark(path_to_index, g_thread_id, 1))
    return true;

  int reparse; // request.is_interactive;
  prev = RawCacheLoad(path_to_index);
  if (!prev)
    reparse = 2;
  else {
    reparse = vfs->Stamp(path_to_index, prev->last_write_time);
    if (FileNeedsParse(*write_time, vfs, request.is_interactive, &*prev,
                       path_to_index, entry.args, std::nullopt))
      reparse = 2;
    for (const auto& dep : prev->dependencies)
      if (auto write_time1 = LastWriteTime(dep.first().str())) {
        if (dep.second < *write_time1) {
          reparse = 2;
          std::lock_guard<std::mutex> lock(vfs->mutex);
          vfs->state[dep.first().str()].stage = 0;
        }
      } else
        reparse = 2;
  }

  if (reparse < 2) {
    PerformanceImportFile perf;
    auto dependencies = prev->dependencies;
    if (reparse) {
      IndexUpdate update = IndexUpdate::CreateDelta(nullptr, prev.get());
      on_indexed->PushBack({std::move(update), perf}, request.is_interactive);
    }
    for (const auto& dep : dependencies)
      if (vfs->Mark(dep.first().str(), 0, 2)) {
        prev = RawCacheLoad(dep.first().str());
        IndexUpdate update = IndexUpdate::CreateDelta(nullptr, prev.get());
        on_indexed->PushBack({std::move(update), perf}, request.is_interactive);
      }

    std::lock_guard<std::mutex> lock(vfs->mutex);
    VFS::State& state = vfs->state[path_to_index];
    if (state.owner == g_thread_id)
      state.stage = 0;
    return true;
  }

  LOG_S(INFO) << "parse " << path_to_index;

  PerformanceImportFile perf;
  auto indexes = indexer->Index(vfs, path_to_index, entry.args, {}, &perf);

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

  for (std::unique_ptr<IndexFile>& curr : indexes) {
    // Only emit diagnostics for non-interactive sessions, which makes it easier
    // to identify indexing problems. For interactive sessions, diagnostics are
    // handled by code completion.
    if (!request.is_interactive)
      diag_engine->Publish(working_files, curr->path, curr->diagnostics_);

    std::string path = curr->path;
    if (!(vfs->Stamp(path, curr->last_write_time) || path == path_to_index))
      continue;
    LOG_S(INFO) << "emit index for " << path;
    prev = RawCacheLoad(path);

    // Write current index to disk if requested.
    LOG_S(INFO) << "store index for " << path;
    Timer time;
    {
      std::string cache_path = GetCachePath(path);
      WriteToFile(cache_path, curr->file_contents);
      WriteToFile(AppendSerializationFormat(cache_path),
                  Serialize(g_config->cacheFormat, *curr));
    }
    perf.index_save_to_disk = time.ElapsedMicrosecondsAndReset();

    vfs->Reset(path_to_index);
    if (entry.id >= 0) {
      std::lock_guard<std::mutex> lock(project->mutex_);
      for (auto& dep : curr->dependencies)
        project->absolute_path_to_entry_index_[dep.first()] = entry.id;
    }

    // Build delta update.
    IndexUpdate update = IndexUpdate::CreateDelta(prev.get(), curr.get());
    perf.index_make_delta = time.ElapsedMicrosecondsAndReset();
    LOG_S(INFO) << "built index for " << path << " (is_delta=" << !!prev << ")";

    on_indexed->PushBack({std::move(update), perf}, request.is_interactive);
  }

  return true;
}

// This function returns true if e2e timing should be displayed for the given
// MethodId.
bool ShouldDisplayMethodTiming(MethodType type) {
  return
    type != kMethodType_TextDocumentPublishDiagnostics &&
    type != kMethodType_CclsPublishInactiveRegions &&
    type != kMethodType_Unknown;
}

}  // namespace

void Init() {
  main_waiter = new MultiQueueWaiter;
  on_request = new ThreadedQueue<std::unique_ptr<InMessage>>(main_waiter);
  on_indexed = new ThreadedQueue<Index_OnIndexed>(main_waiter);

  indexer_waiter = new MultiQueueWaiter;
  index_request = new ThreadedQueue<Index_Request>(indexer_waiter);

  stdout_waiter = new MultiQueueWaiter;
  for_stdout = new ThreadedQueue<Stdout_Request>(stdout_waiter);
}

void Indexer_Main(DiagnosticsEngine* diag_engine,
                  VFS* vfs,
                  Project* project,
                  WorkingFiles* working_files) {
  // Build one index per-indexer, as building the index acquires a global lock.
  ClangIndexer indexer;

  while (true)
    if (!Indexer_Parse(diag_engine, working_files, project, vfs, &indexer))
      indexer_waiter->Wait(index_request);
}

void Main_OnIndexed(DB* db,
                    SemanticHighlightSymbolCache* semantic_cache,
                    WorkingFiles* working_files,
                    Index_OnIndexed* response) {
  if (response->update.refresh) {
    LOG_S(INFO) << "Loaded project. Refresh semantic highlight for all working file.";
    std::lock_guard<std::mutex> lock(working_files->files_mutex);
    for (auto& f : working_files->files) {
      std::string filename = LowerPathIfInsensitive(f->filename);
      if (db->name2file_id.find(filename) == db->name2file_id.end())
        continue;
      QueryFile* file = &db->files[db->name2file_id[filename]];
      EmitSemanticHighlighting(db, semantic_cache, f.get(), file);
    }
    return;
  }

  Timer time;
  db->ApplyIndexUpdate(&response->update);

  // Update indexed content, inactive lines, and semantic highlighting.
  if (response->update.files_def_update) {
    auto& update = *response->update.files_def_update;
    time.ResetAndPrint("apply index for " + update.value.path);
    if (WorkingFile* working_file =
            working_files->GetFileByFilename(update.value.path)) {
      // Update indexed content.
      working_file->SetIndexContent(update.file_content);

      // Inactive lines.
      EmitInactiveLines(working_file, update.value.inactive_regions);

      // Semantic highlighting.
      int file_id =
          db->name2file_id[LowerPathIfInsensitive(working_file->filename)];
      QueryFile* file = &db->files[file_id];
      EmitSemanticHighlighting(db, semantic_cache, working_file, file);
    }
  }
}

void LaunchStdin(std::unordered_map<MethodType, Timer>* request_times) {
  std::thread([request_times]() {
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
      (*request_times)[method_type] = Timer();

      on_request->PushBack(std::move(message));

      // If the message was to exit then querydb will take care of the actual
      // exit. Stop reading from stdin since it might be detached.
      if (method_type == kMethodType_Exit)
        break;
    }
  }).detach();
}

void LaunchStdout(std::unordered_map<MethodType, Timer>* request_times) {
  std::thread([=]() {
    set_thread_name("stdout");

    while (true) {
      std::vector<Stdout_Request> messages = for_stdout->DequeueAll();
      if (messages.empty()) {
        stdout_waiter->Wait(for_stdout);
        continue;
      }

      for (auto& message : messages) {
        if (ShouldDisplayMethodTiming(message.method)) {
          Timer time = (*request_times)[message.method];
          time.ResetAndPrint("[e2e] Running " + std::string(message.method));
        }

        fwrite(message.content.c_str(), message.content.size(), 1, stdout);
        fflush(stdout);
      }
    }
  }).detach();
}

void MainLoop() {
  Project project;
  SemanticHighlightSymbolCache semantic_cache;
  WorkingFiles working_files;
  VFS vfs;
  DiagnosticsEngine diag_engine;

  ClangCompleteManager clang_complete(
      &project, &working_files,
      [&](std::string path, std::vector<lsDiagnostic> diagnostics) {
        diag_engine.Publish(&working_files, path, diagnostics);
      },
      [](lsRequestId id) {
        if (id.Valid()) {
          Out_Error out;
          out.id = id;
          out.error.code = lsErrorCodes::InternalError;
          out.error.message =
              "Dropping completion request; a newer request "
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
  for (MessageHandler* handler : *MessageHandler::message_handlers) {
    handler->db = &db;
    handler->waiter = indexer_waiter;
    handler->project = &project;
    handler->diag_engine = &diag_engine;
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
    for (auto& message : messages) {
      // TODO: Consider using std::unordered_map to lookup the handler
      for (MessageHandler* handler : *MessageHandler::message_handlers) {
        if (handler->GetMethodType() == message->GetMethodType()) {
          handler->Run(std::move(message));
          break;
        }
      }

      if (message)
        LOG_S(ERROR) << "No handler for " << message->GetMethodType();
    }

    for (int i = 80; i--;) {
      std::optional<Index_OnIndexed> response = on_indexed->TryPopFront();
      if (!response)
        break;
      did_work = true;
      Main_OnIndexed(&db, &semantic_cache, &working_files, &*response);
    }

    // Cleanup and free any unused memory.
    FreeUnusedMemory();

    if (!did_work)
      main_waiter->Wait(on_indexed, on_request);
  }
}

void Index(const std::string& path,
           const std::vector<std::string>& args,
           bool interactive,
           lsRequestId id) {
  index_request->PushBack({path, args, interactive, id}, interactive);
}

std::optional<std::string> LoadCachedFileContents(const std::string& path) {
  return ReadContent(GetCachePath(path));
}

void WriteStdout(MethodType method, lsBaseOutMessage& response) {
  std::ostringstream sstream;
  response.Write(sstream);

  Stdout_Request out;
  out.content = sstream.str();
  out.method = method;
  for_stdout->PushBack(std::move(out));
}

}
