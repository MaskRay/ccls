#include "clang_complete.h"
#include "code_complete_cache.h"
#include "fuzzy_match.h"
#include "include_complete.h"
#include "message_handler.h"
#include "queue_manager.h"
#include "timer.h"
#include "working_files.h"

#include "lex_utils.h"

#include <loguru.hpp>

#include <regex>

namespace {
MethodType kMethodType = "textDocument/completion";

// How a completion was triggered
enum class lsCompletionTriggerKind {
  // Completion was triggered by typing an identifier (24x7 code
  // complete), manual invocation (e.g Ctrl+Space) or via API.
  Invoked = 1,

  // Completion was triggered by a trigger character specified by
  // the `triggerCharacters` properties of the `CompletionRegistrationOptions`.
  TriggerCharacter = 2
};
MAKE_REFLECT_TYPE_PROXY(lsCompletionTriggerKind);

// Contains additional information about the context in which a completion
// request is triggered.
struct lsCompletionContext {
  // How the completion was triggered.
  lsCompletionTriggerKind triggerKind;

  // The trigger character (a single character) that has trigger code complete.
  // Is undefined if `triggerKind !== CompletionTriggerKind.TriggerCharacter`
  optional<std::string> triggerCharacter;
};
MAKE_REFLECT_STRUCT(lsCompletionContext, triggerKind, triggerCharacter);

struct lsCompletionParams : lsTextDocumentPositionParams {
  // The completion context. This is only available it the client specifies to
  // send this using
  // `ClientCapabilities.textDocument.completion.contextSupport === true`
  optional<lsCompletionContext> context;
};
MAKE_REFLECT_STRUCT(lsCompletionParams, textDocument, position, context);

struct In_TextDocumentComplete : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsCompletionParams params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentComplete, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentComplete);

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

void DecorateIncludePaths(const std::smatch& match,
                          std::vector<lsCompletionItem>* items) {
  std::string spaces_after_include = " ";
  if (match[3].compare("include") == 0 && match[5].length())
    spaces_after_include = match[4].str();

  std::string prefix =
      match[1].str() + '#' + match[2].str() + "include" + spaces_after_include;
  std::string suffix = match[7].str();

  for (lsCompletionItem& item : *items) {
    char quote0, quote1;
    if (match[5].compare("<") == 0 ||
        (match[5].length() == 0 && item.use_angle_brackets_))
      quote0 = '<', quote1 = '>';
    else
      quote0 = quote1 = '"';

    item.textEdit->newText =
        prefix + quote0 + item.textEdit->newText + quote1 + suffix;
    item.label = prefix + quote0 + item.label + quote1 + suffix;
    item.filterText = nullopt;
  }
}

struct ParseIncludeLineResult {
  bool ok;
  std::string pattern;
  std::smatch match;
};

ParseIncludeLineResult ParseIncludeLine(const std::string& line) {
  static const std::regex pattern(
      "(\\s*)"        // [1]: spaces before '#'
      "#"             //
      "(\\s*)"        // [2]: spaces after '#'
      "([^\\s\"<]*)"  // [3]: "include"
      "(\\s*)"        // [4]: spaces before quote
      "([\"<])?"      // [5]: the first quote char
      "([^\\s\">]*)"  // [6]: path of file
      "[\">]?"        //
      "(.*)");        // [7]: suffix after quote char
  std::smatch match;
  bool ok = std::regex_match(line, match, pattern);
  std::string text = match[3].str() + match[6].str();
  return {ok, text, match};
}

template <typename T>
char* tofixedbase64(T input, char* out) {
  const char* digits =
      "./0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  int len = (sizeof(T) * 8 - 1) / 6 + 1;
  for (int i = len - 1; i >= 0; i--) {
    out[i] = digits[input % 64];
    input /= 64;
  }
  out[len] = '\0';
  return out;
}

// Pre-filters completion responses before sending to vscode. This results in a
// significantly snappier completion experience as vscode is easily overloaded
// when given 1000+ completion items.
void FilterAndSortCompletionResponse(
    Out_TextDocumentComplete* complete_response,
    const std::string& complete_text,
    bool enable) {
  if (!enable)
    return;

  ScopedPerfTimer timer("FilterAndSortCompletionResponse");

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

  auto finalize = [&]() {
    const size_t kMaxResultSize = 100u;
    if (items.size() > kMaxResultSize) {
      items.resize(kMaxResultSize);
      complete_response->result.isIncomplete = true;
    }

    // Set sortText. Note that this happens after resizing - we could do it
    // before, but then we should also sort by priority.
    char buf[16];
    for (size_t i = 0; i < items.size(); ++i)
      items[i].sortText = tofixedbase64(i, buf);
  };

  // No complete text; don't run any filtering logic except to trim the items.
  if (complete_text.empty()) {
    finalize();
    return;
  }

  // Make sure all items have |filterText| set, code that follow needs it.
  for (auto& item : items) {
    if (!item.filterText)
      item.filterText = item.label;
  }

  // Fuzzy match and remove awful candidates.
  FuzzyMatcher fuzzy(complete_text);
  for (auto& item : items) {
    item.score_ =
        CaseFoldingSubsequenceMatch(complete_text, *item.filterText).first
            ? fuzzy.Match(*item.filterText)
            : FuzzyMatcher::kMinScore;
  }
  items.erase(std::remove_if(items.begin(), items.end(),
                             [](const lsCompletionItem& item) {
                               return item.score_ <= FuzzyMatcher::kMinScore;
                             }),
              items.end());
  std::sort(items.begin(), items.end(),
            [](const lsCompletionItem& lhs, const lsCompletionItem& rhs) {
              if (lhs.score_ != rhs.score_)
                return lhs.score_ > rhs.score_;
              if (lhs.priority_ != rhs.priority_)
                return lhs.priority_ < rhs.priority_;
              if (lhs.filterText->size() != rhs.filterText->size())
                return lhs.filterText->size() < rhs.filterText->size();
              return *lhs.filterText < *rhs.filterText;
            });

  // Trim result.
  finalize();
}

struct Handler_TextDocumentCompletion : MessageHandler {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(std::unique_ptr<InMessage> message) override {
    auto request = std::shared_ptr<In_TextDocumentComplete>(
        static_cast<In_TextDocumentComplete*>(message.release()));

    auto write_empty_result = [request]() {
      Out_TextDocumentComplete out;
      out.id = request->id;
      QueueManager::WriteStdout(kMethodType, out);
    };

    std::string path = request->params.textDocument.uri.GetPath();
    WorkingFile* file = working_files->GetFileByFilename(path);
    if (!file) {
      write_empty_result();
      return;
    }

    // It shouldn't be possible, but sometimes vscode will send queries out
    // of order, ie, we get completion request before buffer content update.
    std::string buffer_line;
    if (request->params.position.line >= 0 &&
        request->params.position.line < file->buffer_lines.size()) {
      buffer_line = file->buffer_lines[request->params.position.line];
    }

    // Check for - and : before completing -> or ::, since vscode does not
    // support multi-character trigger characters.
    if (request->params.context &&
        request->params.context->triggerKind ==
            lsCompletionTriggerKind::TriggerCharacter &&
        request->params.context->triggerCharacter) {
      bool did_fail_check = false;

      std::string character = *request->params.context->triggerCharacter;
      int preceding_index = request->params.position.character - 2;

      // If the character is '"', '<' or '/', make sure that the line starts
      // with '#'.
      if (character == "\"" || character == "<" || character == "/") {
        size_t i = 0;
        while (i < buffer_line.size() && isspace(buffer_line[i]))
          ++i;
        if (i >= buffer_line.size() || buffer_line[i] != '#')
          did_fail_check = true;
      }
      // If the character is > or : and we are at the start of the line, do not
      // show completion results.
      else if ((character == ">" || character == ":") && preceding_index < 0) {
        did_fail_check = true;
      }
      // If the character is > but - does not preced it, or if it is : and :
      // does not preced it, do not show completion results.
      else if (preceding_index >= 0 &&
               preceding_index < (int)buffer_line.size()) {
        char preceding = buffer_line[preceding_index];
        did_fail_check = (preceding != '-' && character == ">") ||
                         (preceding != ':' && character == ":");
      }

      if (did_fail_check) {
        write_empty_result();
        return;
      }
    }

    bool is_global_completion = false;
    std::string existing_completion;
    if (file) {
      request->params.position = file->FindStableCompletionSource(
          request->params.position, &is_global_completion,
          &existing_completion);
    }

    ParseIncludeLineResult result = ParseIncludeLine(buffer_line);

    if (result.ok) {
      Out_TextDocumentComplete out;
      out.id = request->id;

      {
        std::unique_lock<std::mutex> lock(
            include_complete->completion_items_mutex, std::defer_lock);
        if (include_complete->is_scanning)
          lock.lock();
        std::string quote = result.match[5];
        for (auto& item : include_complete->completion_items)
          if (quote.empty() || quote == (item.use_angle_brackets_ ? "<" : "\""))
            out.result.items.push_back(item);
      }

      // Needed by |FilterAndSortCompletionResponse|.
      for (lsCompletionItem& item : out.result.items)
        item.filterText = item.label;

      FilterAndSortCompletionResponse(&out, result.pattern,
                                      config->completion.filterAndSort);
      DecorateIncludePaths(result.match, &out.result.items);

      for (lsCompletionItem& item : out.result.items) {
        item.textEdit->range.start.line = request->params.position.line;
        item.textEdit->range.start.character = 0;
        item.textEdit->range.end.line = request->params.position.line;
        item.textEdit->range.end.character = (int)buffer_line.size();
      }

      QueueManager::WriteStdout(kMethodType, out);
    } else {
      ClangCompleteManager::OnComplete callback = std::bind(
          [this, is_global_completion, existing_completion, request](
              const std::vector<lsCompletionItem>& results,
              bool is_cached_result) {
            Out_TextDocumentComplete out;
            out.id = request->id;
            out.result.items = results;

            // Emit completion results.
            FilterAndSortCompletionResponse(&out, existing_completion,
                                            config->completion.filterAndSort);
            QueueManager::WriteStdout(kMethodType, out);

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
            [this](std::vector<lsCompletionItem> results,
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
        clang_complete->CodeComplete(request->id, request->params,
                                     freshen_global);
      } else if (non_global_code_complete_cache->IsCacheValid(
                     request->params)) {
        non_global_code_complete_cache->WithLock([&]() {
          callback(non_global_code_complete_cache->cached_results_,
                   true /*is_cached_result*/);
        });
      } else {
        clang_complete->CodeComplete(request->id, request->params, callback);
      }
    }
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentCompletion);

}  // namespace
