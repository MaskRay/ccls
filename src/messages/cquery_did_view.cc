#include "clang_complete.h"
#include "message_handler.h"
#include "working_files.h"

namespace {
struct Ipc_CqueryTextDocumentDidView
    : public IpcMessage<Ipc_CqueryTextDocumentDidView> {
  struct Params {
    lsDocumentUri textDocumentUri;
  };

  const static IpcId kIpcId = IpcId::CqueryTextDocumentDidView;
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryTextDocumentDidView::Params, textDocumentUri);
MAKE_REFLECT_STRUCT(Ipc_CqueryTextDocumentDidView, params);
REGISTER_IPC_MESSAGE(Ipc_CqueryTextDocumentDidView);

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
}  // namespace