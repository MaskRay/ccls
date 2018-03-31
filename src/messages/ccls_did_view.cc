#include "clang_complete.h"
#include "message_handler.h"
#include "working_files.h"

namespace {
MethodType kMethodType = "$ccls/textDocumentDidView";

struct In_CclsTextDocumentDidView : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct Params {
    lsDocumentUri textDocumentUri;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(In_CclsTextDocumentDidView::Params, textDocumentUri);
MAKE_REFLECT_STRUCT(In_CclsTextDocumentDidView, params);
REGISTER_IN_MESSAGE(In_CclsTextDocumentDidView);

struct Handler_CclsDidView
    : BaseMessageHandler<In_CclsTextDocumentDidView> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_CclsTextDocumentDidView* request) override {
    std::string path = request->params.textDocumentUri.GetPath();

    WorkingFile* working_file = working_files->GetFileByFilename(path);
    if (!working_file)
      return;
    QueryFile* file = nullptr;
    if (!FindFileOrFail(db, project, std::nullopt, path, &file))
      return;

    clang_complete->NotifyView(path);
    if (file->def) {
      EmitInactiveLines(working_file, file->def->inactive_regions);
      EmitSemanticHighlighting(db, semantic_cache, working_file, file);
    }
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CclsDidView);
}  // namespace
