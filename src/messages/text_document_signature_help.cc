#include "message_handler.h"
#include "timer.h"

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
               const NonElidedVector<lsCompletionItem>& results,
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
          IpcManager::WriteStdout(IpcId::TextDocumentSignatureHelp, out);

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
