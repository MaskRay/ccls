#include "message_handler.h"

#include "lex_utils.h"

struct TextDocumentCompletionHandler : MessageHandler {
  IpcId GetId() const override { return IpcId::TextDocumentCompletion; }

  void Run(std::unique_ptr<BaseIpcMessage> message) override {
    auto request = std::shared_ptr<Ipc_TextDocumentComplete>(
        static_cast<Ipc_TextDocumentComplete*>(message.release()));

    std::string path = request->params.textDocument.uri.GetPath();
    WorkingFile* file = working_files->GetFileByFilename(path);

    // It shouldn't be possible, but sometimes vscode will send queries out
    // of order, ie, we get completion request before buffer content update.
    std::string buffer_line;
    if (request->params.position.line >= 0 &&
        request->params.position.line < file->all_buffer_lines.size()) {
      buffer_line = file->all_buffer_lines[request->params.position.line];
    }

    if (ShouldRunIncludeCompletion(buffer_line)) {
      Out_TextDocumentComplete out;
      out.id = request->id;

      {
        std::unique_lock<std::mutex> lock(
            include_complete->completion_items_mutex, std::defer_lock);
        if (include_complete->is_scanning)
          lock.lock();
        out.result.items.assign(include_complete->completion_items.begin(),
                                include_complete->completion_items.end());
        if (lock)
          lock.unlock();

        // Update textEdit params.
        for (lsCompletionItem& item : out.result.items) {
          item.textEdit->range.start.line = request->params.position.line;
          item.textEdit->range.start.character = 0;
          item.textEdit->range.end.line = request->params.position.line;
          item.textEdit->range.end.character = (int)buffer_line.size();
        }
      }

      FilterCompletionResponse(&out, buffer_line);
      IpcManager::WriteStdout(IpcId::TextDocumentCompletion, out);
    } else {
      bool is_global_completion = false;
      std::string existing_completion;
      if (file) {
        request->params.position = file->FindStableCompletionSource(
            request->params.position, &is_global_completion,
            &existing_completion);
      }

      ClangCompleteManager::OnComplete callback = std::bind(
          [this, is_global_completion, existing_completion, request](
              const NonElidedVector<lsCompletionItem>& results,
              bool is_cached_result) {
            Out_TextDocumentComplete out;
            out.id = request->id;
            out.result.items = results;

            // Emit completion results.
            FilterCompletionResponse(&out, existing_completion);
            IpcManager::WriteStdout(IpcId::TextDocumentCompletion, out);

            // Cache completion results.
            if (!is_cached_result) {
              std::string path = request->params.textDocument.uri.GetPath();
              if (is_global_completion) {
                global_code_complete_cache->WithLock([&]() {
                  global_code_complete_cache->cached_path_ = path;
                  global_code_complete_cache->cached_results_ = results;
                });
              } else {
                non_global_code_complete_cache->WithLock([&]() {
                  non_global_code_complete_cache->cached_path_ = path;
                  non_global_code_complete_cache->cached_completion_position_ =
                      request->params.position;
                  non_global_code_complete_cache->cached_results_ = results;
                });
              }
            }
          },
          std::placeholders::_1, std::placeholders::_2);

      bool is_cache_match = false;
      global_code_complete_cache->WithLock([&]() {
        is_cache_match = is_global_completion &&
                         global_code_complete_cache->cached_path_ == path &&
                         !global_code_complete_cache->cached_results_.empty();
      });
      if (is_cache_match) {
        ClangCompleteManager::OnComplete freshen_global =
            [this](NonElidedVector<lsCompletionItem> results,
                   bool is_cached_result) {
              assert(!is_cached_result);

              // note: path is updated in the normal completion handler.
              global_code_complete_cache->WithLock([&]() {
                global_code_complete_cache->cached_results_ = results;
              });
            };

        global_code_complete_cache->WithLock([&]() {
          callback(global_code_complete_cache->cached_results_,
                   true /*is_cached_result*/);
        });
        clang_complete->CodeComplete(request->params, freshen_global);
      } else if (non_global_code_complete_cache->IsCacheValid(
                     request->params)) {
        non_global_code_complete_cache->WithLock([&]() {
          callback(non_global_code_complete_cache->cached_results_,
                   true /*is_cached_result*/);
        });
      } else {
        clang_complete->CodeComplete(request->params, callback);
      }
    }
  }
};
REGISTER_MESSAGE_HANDLER(TextDocumentCompletionHandler);
