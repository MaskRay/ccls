#include "message_handler.h"

struct CqueryDidViewHandler
    : BaseMessageHandler<Ipc_CqueryTextDocumentDidView> {
  void Run(Ipc_CqueryTextDocumentDidView* request) override {
    std::string path = request->params.textDocumentUri.GetPath();

    WorkingFile* working_file = working_files->GetFileByFilename(path);
    if (!working_file)
      return;
    QueryFile* file = nullptr;
    if (!FindFileOrFail(db, nullopt, path, &file))
      return;

    clang_complete->NotifyView(path);
    if (file->def) {
      EmitInactiveLines(working_file, file->def->inactive_regions);
      EmitSemanticHighlighting(db, semantic_cache, working_file, file);
    }
  }
};
REGISTER_MESSAGE_HANDLER(CqueryDidViewHandler);
