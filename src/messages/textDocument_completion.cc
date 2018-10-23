/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "clang_complete.hh"
#include "fuzzy_match.h"
#include "include_complete.h"
#include "log.hh"
#include "message_handler.h"
#include "pipeline.hh"
#include "working_files.h"

#include <clang/Sema/CodeCompleteConsumer.h>
#include <clang/Sema/Sema.h>

#include <regex>

using namespace ccls;
using namespace clang;
using namespace llvm;

namespace {
MethodType kMethodType = "textDocument/completion";

// How a completion was triggered
enum class lsCompletionTriggerKind {
  // Completion was triggered by typing an identifier (24x7 code
  // complete), manual invocation (e.g Ctrl+Space) or via API.
  Invoked = 1,
  // Completion was triggered by a trigger character specified by
  // the `triggerCharacters` properties of the `CompletionRegistrationOptions`.
  TriggerCharacter = 2,
  // Completion was re-triggered as the current completion list is incomplete.
  TriggerForIncompleteCompletions = 3,
};
MAKE_REFLECT_TYPE_PROXY(lsCompletionTriggerKind);

// Contains additional information about the context in which a completion
// request is triggered.
struct lsCompletionContext {
  // How the completion was triggered.
  lsCompletionTriggerKind triggerKind = lsCompletionTriggerKind::Invoked;

  // The trigger character (a single character) that has trigger code complete.
  // Is undefined if `triggerKind !== CompletionTriggerKind.TriggerCharacter`
  std::optional<std::string> triggerCharacter;
};
MAKE_REFLECT_STRUCT(lsCompletionContext, triggerKind, triggerCharacter);

struct lsCompletionParams : lsTextDocumentPositionParams {
  // The completion context. This is only available it the client specifies to
  // send this using
  // `ClientCapabilities.textDocument.completion.contextSupport === true`
  lsCompletionContext context;
};
MAKE_REFLECT_STRUCT(lsCompletionParams, textDocument, position, context);

struct In_TextDocumentComplete : public RequestMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsCompletionParams params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentComplete, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentComplete);

struct lsCompletionList {
  // This list it not complete. Further typing should result in recomputing
  // this list.
  bool isIncomplete = false;
  // The completion items.
  std::vector<lsCompletionItem> items;
};
MAKE_REFLECT_STRUCT(lsCompletionList, isIncomplete, items);

void DecorateIncludePaths(const std::smatch &match,
                          std::vector<lsCompletionItem> *items) {
  std::string spaces_after_include = " ";
  if (match[3].compare("include") == 0 && match[5].length())
    spaces_after_include = match[4].str();

  std::string prefix =
      match[1].str() + '#' + match[2].str() + "include" + spaces_after_include;
  std::string suffix = match[7].str();

  for (lsCompletionItem &item : *items) {
    char quote0, quote1;
    if (match[5].compare("<") == 0 ||
        (match[5].length() == 0 && item.use_angle_brackets_))
      quote0 = '<', quote1 = '>';
    else
      quote0 = quote1 = '"';

    item.textEdit.newText =
        prefix + quote0 + item.textEdit.newText + quote1 + suffix;
    item.label = prefix + quote0 + item.label + quote1 + suffix;
    item.filterText = std::nullopt;
  }
}

struct ParseIncludeLineResult {
  bool ok;
  std::string keyword;
  std::string quote;
  std::string pattern;
  std::smatch match;
};

ParseIncludeLineResult ParseIncludeLine(const std::string &line) {
  static const std::regex pattern("(\\s*)"       // [1]: spaces before '#'
                                  "#"            //
                                  "(\\s*)"       // [2]: spaces after '#'
                                  "([^\\s\"<]*)" // [3]: "include"
                                  "(\\s*)"       // [4]: spaces before quote
                                  "([\"<])?"     // [5]: the first quote char
                                  "([^\\s\">]*)" // [6]: path of file
                                  "[\">]?"       //
                                  "(.*)");       // [7]: suffix after quote char
  std::smatch match;
  bool ok = std::regex_match(line, match, pattern);
  return {ok, match[3], match[5], match[6], match};
}

template <typename T> char *tofixedbase64(T input, char *out) {
  const char *digits = "./0123456789"
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
void FilterCandidates(lsCompletionList &result,
                      const std::string &complete_text, lsPosition begin_pos,
                      lsPosition end_pos, const std::string &buffer_line) {
  assert(begin_pos.line == end_pos.line);
  auto &items = result.items;

  // People usually does not want to insert snippets or parenthesis when
  // changing function or type names, e.g. "str.|()" or "std::|<int>".
  bool has_open_paren = false;
  for (int c = end_pos.character; c < buffer_line.size(); ++c) {
    if (buffer_line[c] == '(' || buffer_line[c] == '<')
      has_open_paren = true;
    if (!isspace(buffer_line[c]))
      break;
  }

  auto finalize = [&]() {
    int max_num = g_config->completion.maxNum;
    if (items.size() > max_num) {
      items.resize(max_num);
      result.isIncomplete = true;
    }

    for (auto &item : items) {
      item.textEdit.range = lsRange{begin_pos, end_pos};
      if (has_open_paren && item.filterText)
        item.textEdit.newText = item.filterText.value();
      // https://github.com/Microsoft/language-server-protocol/issues/543
      // Order of textEdit and additionalTextEdits is unspecified.
      auto &edits = item.additionalTextEdits;
      if (edits.size() && edits[0].range.end == begin_pos) {
        lsPosition start = edits[0].range.start, end = edits[0].range.end;
        item.textEdit.range.start = start;
        item.textEdit.newText = edits[0].newText + item.textEdit.newText;
        if (start.line == begin_pos.line && item.filterText) {
          item.filterText =
              buffer_line.substr(start.character,
                                 end.character - start.character) +
              item.filterText.value();
        }
        edits.erase(edits.begin());
      }
      // Compatibility
      item.insertText = item.textEdit.newText;
    }

    // Set sortText. Note that this happens after resizing - we could do it
    // before, but then we should also sort by priority.
    char buf[16];
    for (size_t i = 0; i < items.size(); ++i)
      items[i].sortText = tofixedbase64(i, buf);
  };

  // No complete text; don't run any filtering logic except to trim the items.
  if (!g_config->completion.filterAndSort || complete_text.empty()) {
    finalize();
    return;
  }

  // Make sure all items have |filterText| set, code that follow needs it.
  for (auto &item : items) {
    if (!item.filterText)
      item.filterText = item.label;
  }

  // Fuzzy match and remove awful candidates.
  bool sensitive = g_config->completion.caseSensitivity;
  FuzzyMatcher fuzzy(complete_text, sensitive);
  for (auto &item : items) {
    item.score_ =
        ReverseSubseqMatch(complete_text, *item.filterText, sensitive) >= 0
            ? fuzzy.Match(*item.filterText)
            : FuzzyMatcher::kMinScore;
  }
  items.erase(std::remove_if(items.begin(), items.end(),
                             [](const lsCompletionItem &item) {
                               return item.score_ <= FuzzyMatcher::kMinScore;
                             }),
              items.end());
  std::sort(items.begin(), items.end(),
            [](const lsCompletionItem &lhs, const lsCompletionItem &rhs) {
              int t = int(lhs.additionalTextEdits.size() -
                          rhs.additionalTextEdits.size());
              if (t)
                return t < 0;
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

lsCompletionItemKind GetCompletionKind(CXCursorKind cursor_kind) {
  switch (cursor_kind) {
  case CXCursor_UnexposedDecl:
    return lsCompletionItemKind::Text;

  case CXCursor_StructDecl:
  case CXCursor_UnionDecl:
    return lsCompletionItemKind::Struct;
  case CXCursor_ClassDecl:
    return lsCompletionItemKind::Class;
  case CXCursor_EnumDecl:
    return lsCompletionItemKind::Enum;
  case CXCursor_FieldDecl:
    return lsCompletionItemKind::Field;
  case CXCursor_EnumConstantDecl:
    return lsCompletionItemKind::EnumMember;
  case CXCursor_FunctionDecl:
    return lsCompletionItemKind::Function;
  case CXCursor_VarDecl:
  case CXCursor_ParmDecl:
    return lsCompletionItemKind::Variable;
  case CXCursor_ObjCInterfaceDecl:
    return lsCompletionItemKind::Interface;

  case CXCursor_ObjCInstanceMethodDecl:
  case CXCursor_CXXMethod:
  case CXCursor_ObjCClassMethodDecl:
    return lsCompletionItemKind::Method;

  case CXCursor_FunctionTemplate:
    return lsCompletionItemKind::Function;

  case CXCursor_Constructor:
  case CXCursor_Destructor:
  case CXCursor_ConversionFunction:
    return lsCompletionItemKind::Constructor;

  case CXCursor_ObjCIvarDecl:
    return lsCompletionItemKind::Variable;

  case CXCursor_ClassTemplate:
  case CXCursor_ClassTemplatePartialSpecialization:
  case CXCursor_UsingDeclaration:
  case CXCursor_TypedefDecl:
  case CXCursor_TypeAliasDecl:
  case CXCursor_TypeAliasTemplateDecl:
  case CXCursor_ObjCCategoryDecl:
  case CXCursor_ObjCProtocolDecl:
  case CXCursor_ObjCImplementationDecl:
  case CXCursor_ObjCCategoryImplDecl:
    return lsCompletionItemKind::Class;

  case CXCursor_ObjCPropertyDecl:
    return lsCompletionItemKind::Property;

  case CXCursor_MacroInstantiation:
  case CXCursor_MacroDefinition:
    return lsCompletionItemKind::Interface;

  case CXCursor_Namespace:
  case CXCursor_NamespaceAlias:
  case CXCursor_NamespaceRef:
    return lsCompletionItemKind::Module;

  case CXCursor_MemberRef:
  case CXCursor_TypeRef:
  case CXCursor_ObjCSuperClassRef:
  case CXCursor_ObjCProtocolRef:
  case CXCursor_ObjCClassRef:
    return lsCompletionItemKind::Reference;

    // return lsCompletionItemKind::Unit;
    // return lsCompletionItemKind::Value;
    // return lsCompletionItemKind::Keyword;
    // return lsCompletionItemKind::Snippet;
    // return lsCompletionItemKind::Color;
    // return lsCompletionItemKind::File;

  case CXCursor_NotImplemented:
  case CXCursor_OverloadCandidate:
    return lsCompletionItemKind::Text;

  case CXCursor_TemplateTypeParameter:
  case CXCursor_TemplateTemplateParameter:
    return lsCompletionItemKind::TypeParameter;

  default:
    LOG_S(WARNING) << "Unhandled completion kind " << cursor_kind;
    return lsCompletionItemKind::Text;
  }
}

void BuildItem(const CodeCompletionResult &R, const CodeCompletionString &CCS,
               std::vector<lsCompletionItem> &out) {
  assert(!out.empty());
  auto first = out.size() - 1;
  bool ignore = false;
  std::string result_type;

  for (const auto &Chunk : CCS) {
    CodeCompletionString::ChunkKind Kind = Chunk.Kind;
    std::string text;
    switch (Kind) {
    case CodeCompletionString::CK_TypedText:
      text = Chunk.Text;
      for (auto i = first; i < out.size(); i++)
        if (Kind == CodeCompletionString::CK_TypedText && !out[i].filterText)
          out[i].filterText = text;
      break;
    case CodeCompletionString::CK_Placeholder:
      text = Chunk.Text;
      for (auto i = first; i < out.size(); i++)
        out[i].parameters_.push_back(text);
      break;
    case CodeCompletionString::CK_Informative:
      if (StringRef(Chunk.Text).endswith("::"))
        continue;
      text = Chunk.Text;
      break;
    case CodeCompletionString::CK_ResultType:
      result_type = Chunk.Text;
      continue;
    case CodeCompletionString::CK_CurrentParameter:
      // This should never be present while collecting completion items.
      llvm_unreachable("unexpected CK_CurrentParameter");
      continue;
    case CodeCompletionString::CK_Optional: {
      // Duplicate last element, the recursive call will complete it.
      if (g_config->completion.duplicateOptional) {
        out.push_back(out.back());
        BuildItem(R, *Chunk.Optional, out);
      }
      continue;
    }
    default:
      text = Chunk.Text;
      break;
    }

    for (auto i = first; i < out.size(); ++i) {
      out[i].label += text;
      if (ignore ||
          (!g_config->client.snippetSupport && out[i].parameters_.size()))
        continue;

      if (Kind == CodeCompletionString::CK_Placeholder) {
        if (R.Kind == CodeCompletionResult::RK_Pattern) {
          ignore = true;
          continue;
        }
        out[i].textEdit.newText +=
            "${" + std::to_string(out[i].parameters_.size()) + ":" + text + "}";
        out[i].insertTextFormat = lsInsertTextFormat::Snippet;
      } else if (Kind != CodeCompletionString::CK_Informative) {
        out[i].textEdit.newText += text;
      }
    }
  }

  if (result_type.size())
    for (auto i = first; i < out.size(); ++i) {
      // ' : ' for variables,
      // ' -> ' (trailing return type-like) for functions
      out[i].label += (out[i].label == out[i].filterText ? " : " : " -> ");
      out[i].label += result_type;
    }
}

class CompletionConsumer : public CodeCompleteConsumer {
  std::shared_ptr<clang::GlobalCodeCompletionAllocator> Alloc;
  CodeCompletionTUInfo CCTUInfo;

public:
  bool from_cache;
  std::vector<lsCompletionItem> ls_items;

  CompletionConsumer(const CodeCompleteOptions &Opts, bool from_cache)
      : CodeCompleteConsumer(Opts, false),
        Alloc(std::make_shared<clang::GlobalCodeCompletionAllocator>()),
        CCTUInfo(Alloc), from_cache(from_cache) {}

  void ProcessCodeCompleteResults(Sema &S, CodeCompletionContext Context,
                                  CodeCompletionResult *Results,
                                  unsigned NumResults) override {
    if (Context.getKind() == CodeCompletionContext::CCC_Recovery)
      return;
    ls_items.reserve(NumResults);
    for (unsigned i = 0; i != NumResults; i++) {
      auto &R = Results[i];
      if (R.Availability == CXAvailability_NotAccessible ||
          R.Availability == CXAvailability_NotAvailable)
        continue;
      if (R.Declaration) {
        if (R.Declaration->getKind() == Decl::CXXDestructor)
          continue;
        if (auto *RD = dyn_cast<RecordDecl>(R.Declaration))
          if (RD->isInjectedClassName())
            continue;
        auto NK = R.Declaration->getDeclName().getNameKind();
        if (NK == DeclarationName::CXXOperatorName ||
            NK == DeclarationName::CXXLiteralOperatorName)
          continue;
      }
      CodeCompletionString *CCS = R.CreateCodeCompletionString(
          S, Context, getAllocator(), getCodeCompletionTUInfo(),
          includeBriefComments());
      lsCompletionItem ls_item;
      ls_item.kind = GetCompletionKind(R.CursorKind);
      if (const char *brief = CCS->getBriefComment())
        ls_item.documentation = brief;
      ls_item.detail = CCS->getParentContextName().str();

      size_t first_idx = ls_items.size();
      ls_items.push_back(ls_item);
      BuildItem(R, *CCS, ls_items);

      for (size_t j = first_idx; j < ls_items.size(); j++) {
        if (g_config->client.snippetSupport &&
            ls_items[j].insertTextFormat == lsInsertTextFormat::Snippet)
          ls_items[j].textEdit.newText += "$0";
        ls_items[j].priority_ = CCS->getPriority();
        if (!g_config->completion.detailedLabel) {
          ls_items[j].detail = ls_items[j].label;
          ls_items[j].label = ls_items[j].filterText.value_or("");
        }
      }
#if LLVM_VERSION_MAJOR >= 7
      for (const FixItHint &FixIt : R.FixIts) {
        auto &AST = S.getASTContext();
        lsTextEdit ls_edit =
            ccls::ToTextEdit(AST.getSourceManager(), AST.getLangOpts(), FixIt);
        for (size_t j = first_idx; j < ls_items.size(); j++)
          ls_items[j].additionalTextEdits.push_back(ls_edit);
      }
#endif
    }
  }

  CodeCompletionAllocator &getAllocator() override { return *Alloc; }
  CodeCompletionTUInfo &getCodeCompletionTUInfo() override { return CCTUInfo; }
};

struct Handler_TextDocumentCompletion
    : BaseMessageHandler<In_TextDocumentComplete> {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(In_TextDocumentComplete *request) override {
    static CompleteConsumerCache<std::vector<lsCompletionItem>> cache;

    const auto &params = request->params;
    lsCompletionList result;

    std::string path = params.textDocument.uri.GetPath();
    WorkingFile *file = working_files->GetFileByFilename(path);
    if (!file) {
      pipeline::Reply(request->id, result);
      return;
    }

    // It shouldn't be possible, but sometimes vscode will send queries out
    // of order, ie, we get completion request before buffer content update.
    std::string buffer_line;
    if (params.position.line >= 0 &&
        params.position.line < file->buffer_lines.size())
      buffer_line = file->buffer_lines[params.position.line];

    // Check for - and : before completing -> or ::, since vscode does not
    // support multi-character trigger characters.
    if (params.context.triggerKind ==
            lsCompletionTriggerKind::TriggerCharacter &&
        params.context.triggerCharacter) {
      bool did_fail_check = false;

      std::string character = *params.context.triggerCharacter;
      int preceding_index = params.position.character - 2;

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
        pipeline::Reply(request->id, result);
        return;
      }
    }

    std::string completion_text;
    lsPosition end_pos = params.position;
    lsPosition begin_pos = file->FindStableCompletionSource(
        params.position, &completion_text, &end_pos);

    ParseIncludeLineResult preprocess = ParseIncludeLine(buffer_line);

    if (preprocess.ok && preprocess.keyword.compare("include") == 0) {
      lsCompletionList result;
      {
        std::unique_lock<std::mutex> lock(
            include_complete->completion_items_mutex, std::defer_lock);
        if (include_complete->is_scanning)
          lock.lock();
        std::string quote = preprocess.match[5];
        for (auto &item : include_complete->completion_items)
          if (quote.empty() || quote == (item.use_angle_brackets_ ? "<" : "\""))
            result.items.push_back(item);
      }
      begin_pos.character = 0;
      end_pos.character = (int)buffer_line.size();
      FilterCandidates(result, preprocess.pattern, begin_pos, end_pos,
                       buffer_line);
      DecorateIncludePaths(preprocess.match, &result.items);
      pipeline::Reply(request->id, result);
    } else {
      std::string path = params.textDocument.uri.GetPath();
      CompletionManager::OnComplete callback =
          [completion_text, path, begin_pos, end_pos,
           id = request->id, buffer_line](CodeCompleteConsumer *OptConsumer) {
            if (!OptConsumer)
              return;
            auto *Consumer = static_cast<CompletionConsumer *>(OptConsumer);
            lsCompletionList result;
            result.items = Consumer->ls_items;

            FilterCandidates(result, completion_text, begin_pos, end_pos,
                             buffer_line);
            pipeline::Reply(id, result);
            if (!Consumer->from_cache) {
              cache.WithLock([&]() {
                cache.path = path;
                cache.position = begin_pos;
                cache.result = Consumer->ls_items;
              });
            }
          };

      clang::CodeCompleteOptions CCOpts;
      CCOpts.IncludeBriefComments = true;
      CCOpts.IncludeCodePatterns = preprocess.ok; // if there is a #
#if LLVM_VERSION_MAJOR >= 7
      CCOpts.IncludeFixIts = true;
#endif
      CCOpts.IncludeMacros = true;
      if (cache.IsCacheValid(path, begin_pos)) {
        CompletionConsumer Consumer(CCOpts, true);
        cache.WithLock([&]() { Consumer.ls_items = cache.result; });
        callback(&Consumer);
      } else {
        clang_complete->completion_request_.PushBack(
          std::make_unique<CompletionManager::CompletionRequest>(
            request->id, params.textDocument, begin_pos,
            std::make_unique<CompletionConsumer>(CCOpts, false), CCOpts,
            callback));
      }
    }
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentCompletion);

} // namespace
