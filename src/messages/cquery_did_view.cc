#include "clang_complete.h"
#include "message_handler.h"
#include "working_files.h"

namespace {
MethodType kMethodType = "$cquery/textDocumentDidView";

struct In_CqueryTextDocumentDidView : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct Params {
    lsDocumentUri textDocumentUri;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(In_CqueryTextDocumentDidView::Params, textDocumentUri);
MAKE_REFLECT_STRUCT(In_CqueryTextDocumentDidView, params);
REGISTER_IN_MESSAGE(In_CqueryTextDocumentDidView);

struct Handler_CqueryDidView
    : BaseMessageHandler<In_CqueryTextDocumentDidView> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_CqueryTextDocumentDidView* request) override {
    std::string path = request->params.textDocumentUri.GetPath();

    WorkingFile* working_file = working_files->GetFileByFilename(path);
    if (!working_file)
      return;
    QueryFile* file = nullptr;
    if (!FindFileOrFail(db, project, nullopt, path, &file))
      return;

    clang_complete->NotifyView(path);
    if (file->def) {
      EmitInactiveLines(working_file, file->def->inactive_regions);
      EmitSemanticHighlighting(db, semantic_cache, working_file, file);
    }
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CqueryDidView);
}  // namespace
