#include "cache.h"
#include "message_handler.h"
#include "timer.h"

// Open, view, change, close file
struct Ipc_TextDocumentDidOpen : public IpcMessage<Ipc_TextDocumentDidOpen> {
  struct Params {
    lsTextDocumentItem textDocument;
  };

  const static IpcId kIpcId = IpcId::TextDocumentDidOpen;
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidOpen::Params, textDocument);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidOpen, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentDidOpen);

struct TextDocumentDidOpenHandler
    : BaseMessageHandler<Ipc_TextDocumentDidOpen> {
  void Run(Ipc_TextDocumentDidOpen* request) override {
    // NOTE: This function blocks code lens. If it starts taking a long time
    // we will need to find a way to unblock the code lens request.

    Timer time;
    std::string path = request->params.textDocument.uri.GetPath();
    WorkingFile* working_file =
        working_files->OnOpen(request->params.textDocument);
    optional<std::string> cached_file_contents =
        LoadCachedFileContents(config, path);
    if (cached_file_contents)
      working_file->SetIndexContent(*cached_file_contents);
    else
      working_file->SetIndexContent(working_file->buffer_content);

    QueryFile* file = nullptr;
    FindFileOrFail(db, nullopt, path, &file);
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
    const Project::Entry& entry = project->FindCompilationEntryForFile(path);
    queue->index_request.PriorityEnqueue(Index_Request(
        entry.filename, entry.args, true /*is_interactive*/, nullopt));
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentDidOpenHandler);
