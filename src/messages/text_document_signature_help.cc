#include "clang_complete.h"
#include "code_complete_cache.h"
#include "message_handler.h"
#include "queue_manager.h"
#include "timer.h"

namespace {
struct Ipc_TextDocumentSignatureHelp
    : public RequestMessage<Ipc_TextDocumentSignatureHelp> {
  const static IpcId kIpcId = IpcId::TextDocumentSignatureHelp;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentSignatureHelp, id, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentSignatureHelp);

// Represents a parameter of a callable-signature. A parameter can
// have a label and a doc-comment.
struct lsParameterInformation {
  // The label of this parameter. Will be shown in
  // the UI.
  std::string label;

  // The human-readable doc-comment of this parameter. Will be shown
  // in the UI but can be omitted.
  optional<std::string> documentation;
};
MAKE_REFLECT_STRUCT(lsParameterInformation, label, documentation);

// Represents the signature of something callable. A signature
// can have a label, like a function-name, a doc-comment, and
// a set of parameters.
struct lsSignatureInformation {
  // The label of this signature. Will be shown in
  // the UI.
  std::string label;

  // The human-readable doc-comment of this signature. Will be shown
  // in the UI but can be omitted.
  optional<std::string> documentation;

  // The parameters of this signature.
  std::vector<lsParameterInformation> parameters;
};
MAKE_REFLECT_STRUCT(lsSignatureInformation, label, documentation, parameters);

// Signature help represents the signature of something
// callable. There can be multiple signature but only one
// active and only one active parameter.
struct lsSignatureHelp {
  // One or more signatures.
  std::vector<lsSignatureInformation> signatures;

  // The active signature. If omitted or the value lies outside the
  // range of `signatures` the value defaults to zero or is ignored if
  // `signatures.length === 0`. Whenever possible implementors should
  // make an active decision about the active signature and shouldn't
  // rely on a default value.
  // In future version of the protocol this property might become
  // mandantory to better express this.
  optional<int> activeSignature;

  // The active parameter of the active signature. If omitted or the value
  // lies outside the range of `signatures[activeSignature].parameters`
  // defaults to 0 if the active signature has parameters. If
  // the active signature has no parameters it is ignored.
  // In future version of the protocol this property might become
  // mandantory to better express the active parameter if the
  // active signature does have any.
  optional<int> activeParameter;
};
MAKE_REFLECT_STRUCT(lsSignatureHelp,
                    signatures,
                    activeSignature,
                    activeParameter);

struct Out_TextDocumentSignatureHelp
    : public lsOutMessage<Out_TextDocumentSignatureHelp> {
  lsRequestId id;
  lsSignatureHelp result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentSignatureHelp, jsonrpc, id, result);

struct TextDocumentSignatureHelpHandler : MessageHandler {
  IpcId GetId() const override { return IpcId::TextDocumentSignatureHelp; }

  void Run(std::unique_ptr<BaseIpcMessage> message) override {
    auto request = message->As<Ipc_TextDocumentSignatureHelp>();
    lsTextDocumentPositionParams& params = request->params;
    WorkingFile* file =
        working_files->GetFileByFilename(params.textDocument.uri.GetPath());
    std::string search;
    int active_param = 0;
    if (file) {
      lsPosition completion_position;
      search = file->FindClosestCallNameInBuffer(params.position, &active_param,
                                                 &completion_position);
      params.position = completion_position;
    }
    if (search.empty())
      return;

    ClangCompleteManager::OnComplete callback = std::bind(
        [this](BaseIpcMessage* message, std::string search, int active_param,
               const std::vector<lsCompletionItem>& results,
               bool is_cached_result) {
          auto msg = message->As<Ipc_TextDocumentSignatureHelp>();

          Out_TextDocumentSignatureHelp out;
          out.id = msg->id;

          for (auto& result : results) {
            if (result.label != search)
              continue;

            lsSignatureInformation signature;
            signature.label = result.detail;
            for (auto& parameter : result.parameters_) {
              lsParameterInformation ls_param;
              ls_param.label = parameter;
              signature.parameters.push_back(ls_param);
            }
            out.result.signatures.push_back(signature);
          }

          // Guess the signature the user wants based on available parameter
          // count.
          out.result.activeSignature = 0;
          for (size_t i = 0; i < out.result.signatures.size(); ++i) {
            if (active_param < out.result.signatures.size()) {
              out.result.activeSignature = (int)i;
              break;
            }
          }

          // Set signature to what we parsed from the working file.
          out.result.activeParameter = active_param;

          Timer timer;
          QueueManager::WriteStdout(IpcId::TextDocumentSignatureHelp, out);

          if (!is_cached_result) {
            signature_cache->WithLock([&]() {
              signature_cache->cached_path_ =
                  msg->params.textDocument.uri.GetPath();
              signature_cache->cached_completion_position_ =
                  msg->params.position;
              signature_cache->cached_results_ = results;
            });
          }

          delete message;
        },
        message.release(), search, active_param, std::placeholders::_1,
        std::placeholders::_2);

    if (signature_cache->IsCacheValid(params)) {
      signature_cache->WithLock([&]() {
        callback(signature_cache->cached_results_, true /*is_cached_result*/);
      });
    } else {
      clang_complete->CodeComplete(params, std::move(callback));
    }
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentSignatureHelpHandler);
}  // namespace
