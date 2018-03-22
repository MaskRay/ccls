#include "cache_manager.h"
#include "clang_complete.h"
#include "include_complete.h"
#include "message_handler.h"
#include "project.h"
#include "queue_manager.h"
#include "timer.h"
#include "working_files.h"

#include <loguru.hpp>

namespace {
MethodType kMethodType = "textDocument/didOpen";

// Open, view, change, close file
struct In_TextDocumentDidOpen : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  
  struct Params {
    lsTextDocumentItem textDocument;

    // cquery extension
    // If specified (e.g. ["clang++", "-DM", "a.cc"]), it overrides the project
    // entry (e.g. loaded from compile_commands.json or .cquery).
    std::vector<std::string> args;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentDidOpen::Params, textDocument, args);
MAKE_REFLECT_STRUCT(In_TextDocumentDidOpen, params);
REGISTER_IN_MESSAGE(In_TextDocumentDidOpen);

struct Handler_TextDocumentDidOpen
    : BaseMessageHandler<In_TextDocumentDidOpen> {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(In_TextDocumentDidOpen* request) override {
    // NOTE: This function blocks code lens. If it starts taking a long time
    // we will need to find a way to unblock the code lens request.
    const auto& params = request->params;
    Timer time;
    std::string path = params.textDocument.uri.GetPath();
    if (ShouldIgnoreFileForIndexing(path))
      return;

    std::shared_ptr<ICacheManager> cache_manager = ICacheManager::Make(config);
    WorkingFile* working_file = working_files->OnOpen(params.textDocument);
    optional<std::string> cached_file_contents =
        cache_manager->LoadCachedFileContents(path);
    if (cached_file_contents)
      working_file->SetIndexContent(*cached_file_contents);

    QueryFile* file = nullptr;
    FindFileOrFail(db, project, nullopt, path, &file);
    if (file && file->def) {
      EmitInactiveLines(working_file, file->def->inactive_regions);
      EmitSemanticHighlighting(db, semantic_cache, working_file, file);
    }

    time.ResetAndPrint(
        "[querydb] Loading cached index file for DidOpen (blocks "
        "CodeLens)");

    include_complete->AddFile(working_file->filename);
    clang_complete->NotifyView(path);

    // Submit new index request.
    Project::Entry entry = project->FindCompilationEntryForFile(path);
    QueueManager::instance()->index_request.PushBack(
        Index_Request(
            entry.filename, params.args.size() ? params.args : entry.args,
            true /*is_interactive*/, params.textDocument.text, cache_manager),
        true /* priority */);

    clang_complete->FlushSession(entry.filename);
    LOG_S(INFO) << "Flushed clang complete sessions for " << entry.filename;
    if (params.args.size()) {
      project->SetFlagsForFile(params.args, path);
    }
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDidOpen);
}  // namespace
