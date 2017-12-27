#include "message_handler.h"

#include "lex_utils.h"

#include <loguru.hpp>

namespace {

struct Ipc_TextDocumentComplete : public IpcMessage<Ipc_TextDocumentComplete> {
  const static IpcId kIpcId = IpcId::TextDocumentCompletion;

  lsRequestId id;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentComplete, id, params);
REGISTER_IPC_MESSAGE(Ipc_TextDocumentComplete);

struct lsTextDocumentCompleteResult {
  // This list it not complete. Further typing should result in recomputing
  // this list.
  bool isIncomplete = false;
  // The completion items.
  std::vector<lsCompletionItem> items;
};
MAKE_REFLECT_STRUCT(lsTextDocumentCompleteResult, isIncomplete, items);

struct Out_TextDocumentComplete
    : public lsOutMessage<Out_TextDocumentComplete> {
  lsRequestId id;
  lsTextDocumentCompleteResult result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentComplete, jsonrpc, id, result);

// Pre-filters completion responses before sending to vscode. This results in a
// significantly snappier completion experience as vscode is easily overloaded
// when given 1000+ completion items.
void FilterCompletionResponse(Out_TextDocumentComplete* complete_response,
                              const std::string& complete_text) {
// Used to inject more completions.
#if false
  const size_t kNumIterations = 250;
  size_t size = complete_response->result.items.size();
  complete_response->result.items.reserve(size * (kNumIterations + 1));
  for (size_t iteration = 0; iteration < kNumIterations; ++iteration) {
    for (size_t i = 0; i < size; ++i) {
      auto item = complete_response->result.items[i];
      item.label += "#" + std::to_string(iteration);
      complete_response->result.items.push_back(item);
    }
  }
#endif

  auto& items = complete_response->result.items;

  // If the text doesn't start with underscore,
  // remove all candidates that start with underscore.
  if (!complete_text.empty() && complete_text[0] != '_') {
    items.erase(std::remove_if(items.begin(), items.end(),
                               [](const lsCompletionItem& item) {
                                 return item.label[0] == '_';
                               }),
                items.end());
  }

  // find the exact text
  const bool found = !complete_text.empty() &&
                     std::find_if(items.begin(), items.end(),
                                  [&](const lsCompletionItem& item) {
                                    return item.label == complete_text;
                                  }) != items.end();
  // If found, remove all candidates that do not start with it.
  if (found) {
    items.erase(std::remove_if(items.begin(), items.end(),
                               [&](const lsCompletionItem& item) {
                                 return item.label.find(complete_text) != 0;
                               }),
                items.end());
  }

  const size_t kMaxResultSize = 100u;
  if (items.size() > kMaxResultSize) {
    if (complete_text.empty()) {
      items.resize(kMaxResultSize);
    } else {
      std::vector<lsCompletionItem> filtered_result;
      filtered_result.reserve(kMaxResultSize);

      std::unordered_set<std::string> inserted;
      inserted.reserve(kMaxResultSize);

      // Find literal matches first.
      for (const auto& item : items) {
        if (item.label.find(complete_text) != std::string::npos) {
          // Don't insert the same completion entry.
          if (!inserted.insert(item.InsertedContent()).second)
            continue;

          filtered_result.push_back(item);
          if (filtered_result.size() >= kMaxResultSize)
            break;
        }
      }

      // Find fuzzy matches if we haven't found all of the literal matches.
      if (filtered_result.size() < kMaxResultSize) {
        for (const auto& item : items) {
          if (SubstringMatch(complete_text, item.label)) {
            // Don't insert the same completion entry.
            if (!inserted.insert(item.InsertedContent()).second)
              continue;

            filtered_result.push_back(item);
            if (filtered_result.size() >= kMaxResultSize)
              break;
          }
        }
      }

      items = filtered_result;
    }

    // Assuming the client does not support out-of-order completion (ie, ao
    // matches against oa), our filtering is guaranteed to contain any
    // potential matches, so the completion is only incomplete if we have the
    // max number of emitted matches.
    if (items.size() >= kMaxResultSize) {
      LOG_S(INFO) << "Marking completion results as incomplete";
      complete_response->result.isIncomplete = true;
    }
  }
}

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
      QueueManager::WriteStdout(IpcId::TextDocumentCompletion, out);
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
              const std::vector<lsCompletionItem>& results,
              bool is_cached_result) {
            Out_TextDocumentComplete out;
            out.id = request->id;
            out.result.items = results;

            // Emit completion results.
            FilterCompletionResponse(&out, existing_completion);
            QueueManager::WriteStdout(IpcId::TextDocumentCompletion, out);

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
        ClangCompleteManager::OnComplete freshen_global = [this](
            std::vector<lsCompletionItem> results, bool is_cached_result) {
          assert(!is_cached_result);

          // note: path is updated in the normal completion handler.
          global_code_complete_cache->WithLock(
              [&]() { global_code_complete_cache->cached_results_ = results; });
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

}  // namespace
