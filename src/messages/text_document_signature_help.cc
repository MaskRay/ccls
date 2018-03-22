#include "clang_complete.h"
#include "code_complete_cache.h"
#include "message_handler.h"
#include "queue_manager.h"
#include "timer.h"

#include <stdint.h>

namespace {
MethodType kMethodType = "textDocument/signatureHelp";

struct In_TextDocumentSignatureHelp : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentSignatureHelp, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentSignatureHelp);

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

struct Handler_TextDocumentSignatureHelp : MessageHandler {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(std::unique_ptr<InMessage> message) override {
    auto request = static_cast<In_TextDocumentSignatureHelp*>(message.get());
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
        [this](InMessage* message, std::string search, int active_param,
               const std::vector<lsCompletionItem>& results,
               bool is_cached_result) {
          auto msg = static_cast<In_TextDocumentSignatureHelp*>(message);

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

          // Prefer the signature with least parameter count but still larger
          // than active_param.
          out.result.activeSignature = 0;
          if (out.result.signatures.size()) {
            size_t num_parameters = SIZE_MAX;
            for (size_t i = 0; i < out.result.signatures.size(); ++i) {
              size_t t = out.result.signatures[i].parameters.size();
              if (active_param < t && t < num_parameters) {
                out.result.activeSignature = int(i);
                num_parameters = t;
              }
            }
          }

          // Set signature to what we parsed from the working file.
          out.result.activeParameter = active_param;

          Timer timer;
          QueueManager::WriteStdout(kMethodType, out);

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
      clang_complete->CodeComplete(request->id, params, std::move(callback));
    }
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentSignatureHelp);
}  // namespace
