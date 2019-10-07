// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "fuzzy_match.hh"
#include "include_complete.hh"
#include "log.hh"
#include "message_handler.hh"
#include "pipeline.hh"
#include "sema_manager.hh"
#include "working_files.hh"

#include <clang/Sema/CodeCompleteConsumer.h>
#include <clang/Sema/Sema.h>
#include <llvm/ADT/Twine.h>

#if LLVM_VERSION_MAJOR < 8
#include <regex>
#endif

namespace ccls {
using namespace clang;
using namespace llvm;

REFLECT_UNDERLYING(InsertTextFormat);
REFLECT_UNDERLYING(CompletionItemKind);

void reflect(JsonWriter &vis, CompletionItem &v) {
  reflectMemberStart(vis);
  REFLECT_MEMBER(label);
  REFLECT_MEMBER(kind);
  REFLECT_MEMBER(detail);
  if (v.documentation.size())
    REFLECT_MEMBER(documentation);
  REFLECT_MEMBER(sortText);
  if (v.filterText.size())
    REFLECT_MEMBER(filterText);
  REFLECT_MEMBER(insertTextFormat);
  REFLECT_MEMBER(textEdit);
  if (v.additionalTextEdits.size())
    REFLECT_MEMBER(additionalTextEdits);
  reflectMemberEnd(vis);
}

namespace {
struct CompletionList {
  bool isIncomplete = false;
  std::vector<CompletionItem> items;
};
REFLECT_STRUCT(CompletionList, isIncomplete, items);

#if LLVM_VERSION_MAJOR < 8
void decorateIncludePaths(const std::smatch &match,
                          std::vector<CompletionItem> *items, char quote) {
  std::string spaces_after_include = " ";
  if (match[3].compare("include") == 0 && quote != '\0')
    spaces_after_include = match[4].str();

  std::string prefix =
      match[1].str() + '#' + match[2].str() + "include" + spaces_after_include;
  std::string suffix = match[7].str();

  for (CompletionItem &item : *items) {
    char quote0, quote1;
    if (quote != '"')
      quote0 = '<', quote1 = '>';
    else
      quote0 = quote1 = '"';

    item.textEdit.newText =
        prefix + quote0 + item.textEdit.newText + quote1 + suffix;
    item.label = prefix + quote0 + item.label + quote1 + suffix;
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
#endif

// Pre-filters completion responses before sending to vscode. This results in a
// significantly snappier completion experience as vscode is easily overloaded
// when given 1000+ completion items.
void filterCandidates(CompletionList &result, const std::string &complete_text,
                      Position begin_pos, Position end_pos,
                      const std::string &buffer_line) {
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

    int overwrite_len = 0;
    for (auto &item : items) {
      auto &edits = item.additionalTextEdits;
      if (edits.size() && edits[0].range.end == begin_pos) {
        Position start = edits[0].range.start, end = edits[0].range.end;
        if (start.line == begin_pos.line) {
          overwrite_len =
              std::max(overwrite_len, end.character - start.character);
        } else {
          overwrite_len = -1;
          break;
        }
      }
    }

    Position overwrite_begin = {begin_pos.line,
                                begin_pos.character - overwrite_len};
    std::string sort(4, ' ');
    for (auto &item : items) {
      item.textEdit.range = lsRange{begin_pos, end_pos};
      if (has_open_paren)
        item.textEdit.newText = item.filterText;
      // https://github.com/Microsoft/language-server-protocol/issues/543
      // Order of textEdit and additionalTextEdits is unspecified.
      auto &edits = item.additionalTextEdits;
      if (overwrite_len > 0) {
        item.textEdit.range.start = overwrite_begin;
        std::string orig =
            buffer_line.substr(overwrite_begin.character, overwrite_len);
        if (edits.size() && edits[0].range.end == begin_pos &&
            edits[0].range.start.line == begin_pos.line) {
          int cur_edit_len =
              edits[0].range.end.character - edits[0].range.start.character;
          item.textEdit.newText =
              buffer_line.substr(overwrite_begin.character,
                                 overwrite_len - cur_edit_len) +
              edits[0].newText + item.textEdit.newText;
          edits.erase(edits.begin());
        } else {
          item.textEdit.newText = orig + item.textEdit.newText;
        }
        item.filterText = orig + item.filterText;
      }
      if (item.filterText == item.label)
        item.filterText.clear();
      for (auto i = sort.size(); i && ++sort[i - 1] == 'A';)
        sort[--i] = ' ';
      item.sortText = sort;
    }
  };

  if (!g_config->completion.filterAndSort) {
    finalize();
    return;
  }

  if (complete_text.size()) {
    // Fuzzy match and remove awful candidates.
    bool sensitive = g_config->completion.caseSensitivity;
    FuzzyMatcher fuzzy(complete_text, sensitive);
    for (CompletionItem &item : items) {
      const std::string &filter =
          item.filterText.size() ? item.filterText : item.label;
      item.score_ = reverseSubseqMatch(complete_text, filter, sensitive) >= 0
                        ? fuzzy.match(filter, true)
                        : FuzzyMatcher::kMinScore;
    }
    items.erase(std::remove_if(items.begin(), items.end(),
                               [](const CompletionItem &item) {
                                 return item.score_ <= FuzzyMatcher::kMinScore;
                               }),
                items.end());
  }
  std::sort(items.begin(), items.end(),
            [](const CompletionItem &lhs, const CompletionItem &rhs) {
              int t = int(lhs.additionalTextEdits.size() -
                          rhs.additionalTextEdits.size());
              if (t)
                return t < 0;
              if (lhs.score_ != rhs.score_)
                return lhs.score_ > rhs.score_;
              if (lhs.priority_ != rhs.priority_)
                return lhs.priority_ < rhs.priority_;
              t = lhs.textEdit.newText.compare(rhs.textEdit.newText);
              if (t)
                return t < 0;
              t = lhs.label.compare(rhs.label);
              if (t)
                return t < 0;
              return lhs.filterText < rhs.filterText;
            });

  // Trim result.
  finalize();
}

CompletionItemKind getCompletionKind(CodeCompletionContext::Kind k,
                                     const CodeCompletionResult &r) {
  switch (r.Kind) {
  case CodeCompletionResult::RK_Declaration: {
    const Decl *d = r.Declaration;
    switch (d->getKind()) {
    case Decl::LinkageSpec:
      return CompletionItemKind::Keyword;
    case Decl::Namespace:
    case Decl::NamespaceAlias:
      return CompletionItemKind::Module;
    case Decl::ObjCCategory:
    case Decl::ObjCCategoryImpl:
    case Decl::ObjCImplementation:
    case Decl::ObjCInterface:
    case Decl::ObjCProtocol:
      return CompletionItemKind::Interface;
    case Decl::ObjCMethod:
      return CompletionItemKind::Method;
    case Decl::ObjCProperty:
      return CompletionItemKind::Property;
    case Decl::ClassTemplate:
      return CompletionItemKind::Class;
    case Decl::FunctionTemplate:
      return CompletionItemKind::Function;
    case Decl::TypeAliasTemplate:
      return CompletionItemKind::Class;
    case Decl::VarTemplate:
      if (cast<VarTemplateDecl>(d)->getTemplatedDecl()->isConstexpr())
        return CompletionItemKind::Constant;
      return CompletionItemKind::Variable;
    case Decl::TemplateTemplateParm:
      return CompletionItemKind::TypeParameter;
    case Decl::Enum:
      return CompletionItemKind::Enum;
    case Decl::CXXRecord:
    case Decl::Record:
      if (auto *rd = dyn_cast<RecordDecl>(d))
        if (rd->getTagKind() == TTK_Struct)
          return CompletionItemKind::Struct;
      return CompletionItemKind::Class;
    case Decl::TemplateTypeParm:
    case Decl::TypeAlias:
    case Decl::Typedef:
      return CompletionItemKind::TypeParameter;
    case Decl::Using:
    case Decl::ConstructorUsingShadow:
      return CompletionItemKind::Keyword;
    case Decl::Binding:
      return CompletionItemKind::Variable;
    case Decl::Field:
    case Decl::ObjCIvar:
      return CompletionItemKind::Field;
    case Decl::Function:
      return CompletionItemKind::Function;
    case Decl::CXXMethod:
      return CompletionItemKind::Method;
    case Decl::CXXConstructor:
      return CompletionItemKind::Constructor;
    case Decl::CXXConversion:
    case Decl::CXXDestructor:
      return CompletionItemKind::Method;
    case Decl::Var:
    case Decl::Decomposition:
    case Decl::ImplicitParam:
    case Decl::ParmVar:
    case Decl::VarTemplateSpecialization:
    case Decl::VarTemplatePartialSpecialization:
      if (cast<VarDecl>(d)->isConstexpr())
        return CompletionItemKind::Constant;
      return CompletionItemKind::Variable;
    case Decl::EnumConstant:
      return CompletionItemKind::EnumMember;
    case Decl::IndirectField:
      return CompletionItemKind::Field;

    default:
      LOG_S(WARNING) << "Unhandled " << int(d->getKind());
      return CompletionItemKind::Text;
    }
    break;
  }
  case CodeCompletionResult::RK_Keyword:
    return CompletionItemKind::Keyword;
  case CodeCompletionResult::RK_Macro:
    return CompletionItemKind::Reference;
  case CodeCompletionResult::RK_Pattern:
#if LLVM_VERSION_MAJOR >= 8
    if (k == CodeCompletionContext::CCC_IncludedFile)
      return CompletionItemKind::File;
#endif
    return CompletionItemKind::Snippet;
  }
}

void buildItem(const CodeCompletionResult &r, const CodeCompletionString &ccs,
               std::vector<CompletionItem> &out) {
  assert(!out.empty());
  auto first = out.size() - 1;
  bool ignore = false;
  std::string result_type;

  for (const auto &chunk : ccs) {
    CodeCompletionString::ChunkKind kind = chunk.Kind;
    std::string text;
    switch (kind) {
    case CodeCompletionString::CK_TypedText:
      text = chunk.Text;
      for (auto i = first; i < out.size(); i++)
        out[i].filterText = text;
      break;
    case CodeCompletionString::CK_Placeholder:
      text = chunk.Text;
      for (auto i = first; i < out.size(); i++)
        out[i].parameters_.push_back(text);
      break;
    case CodeCompletionString::CK_Informative:
      if (StringRef(chunk.Text).endswith("::"))
        continue;
      text = chunk.Text;
      break;
    case CodeCompletionString::CK_ResultType:
      result_type = chunk.Text;
      continue;
    case CodeCompletionString::CK_CurrentParameter:
      // This should never be present while collecting completion items.
      llvm_unreachable("unexpected CK_CurrentParameter");
      continue;
    case CodeCompletionString::CK_Optional: {
      // Duplicate last element, the recursive call will complete it.
      if (g_config->completion.duplicateOptional) {
        out.push_back(out.back());
        buildItem(r, *chunk.Optional, out);
      }
      continue;
    }
    default:
      text = chunk.Text;
      break;
    }

    for (auto i = first; i < out.size(); ++i) {
      out[i].label += text;
      if (ignore ||
          (!g_config->client.snippetSupport && out[i].parameters_.size()))
        continue;

      if (kind == CodeCompletionString::CK_Placeholder) {
        if (r.Kind == CodeCompletionResult::RK_Pattern) {
          ignore = true;
          continue;
        }
        out[i].textEdit.newText +=
            ("${" + Twine(out[i].parameters_.size()) + ":" + text + "}").str();
        out[i].insertTextFormat = InsertTextFormat::Snippet;
      } else if (kind != CodeCompletionString::CK_Informative) {
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
  std::shared_ptr<clang::GlobalCodeCompletionAllocator> alloc;
  CodeCompletionTUInfo cctu_info;

public:
  bool from_cache;
  std::vector<CompletionItem> ls_items;

  CompletionConsumer(const CodeCompleteOptions &opts, bool from_cache)
      :
#if LLVM_VERSION_MAJOR >= 9 // rC358696
        CodeCompleteConsumer(opts),
#else
        CodeCompleteConsumer(opts, false),
#endif
        alloc(std::make_shared<clang::GlobalCodeCompletionAllocator>()),
        cctu_info(alloc), from_cache(from_cache) {
  }

  void ProcessCodeCompleteResults(Sema &s, CodeCompletionContext context,
                                  CodeCompletionResult *results,
                                  unsigned numResults) override {
    if (context.getKind() == CodeCompletionContext::CCC_Recovery)
      return;
    ls_items.reserve(numResults);
    for (unsigned i = 0; i != numResults; i++) {
      auto &r = results[i];
      if (r.Availability == CXAvailability_NotAccessible ||
          r.Availability == CXAvailability_NotAvailable)
        continue;
      if (r.Declaration) {
        Decl::Kind k = r.Declaration->getKind();
        if (k == Decl::CXXDestructor)
          continue;
        if (k == Decl::FunctionTemplate) {
          // Ignore CXXDeductionGuide which has empty TypedText.
          auto *fd = cast<FunctionTemplateDecl>(r.Declaration);
          if (fd->getTemplatedDecl()->getKind() == Decl::CXXDeductionGuide)
            continue;
        }
        if (auto *rd = dyn_cast<RecordDecl>(r.Declaration))
          if (rd->isInjectedClassName())
            continue;
        auto nk = r.Declaration->getDeclName().getNameKind();
        if (nk == DeclarationName::CXXOperatorName ||
            nk == DeclarationName::CXXLiteralOperatorName)
          continue;
      }

      CodeCompletionString *ccs = r.CreateCodeCompletionString(
          s, context, getAllocator(), getCodeCompletionTUInfo(),
          includeBriefComments());
      CompletionItem ls_item;
      ls_item.kind = getCompletionKind(context.getKind(), r);
      if (const char *brief = ccs->getBriefComment())
        ls_item.documentation = brief;
      ls_item.detail = ccs->getParentContextName().str();

      size_t first_idx = ls_items.size();
      ls_items.push_back(ls_item);
      buildItem(r, *ccs, ls_items);

      for (size_t j = first_idx; j < ls_items.size(); j++) {
        std::string &s = ls_items[j].textEdit.newText;
        if (!g_config->client.snippetSupport) {
          if (s.size()) {
            // Delete non-identifier parts.
            if (s.back() == '(' || s.back() == '<')
              s.pop_back();
            else if (s.size() >= 2 && !s.compare(s.size() - 2, 2, "()"))
              s.resize(s.size() - 2);
          }
        } else if (ls_items[j].insertTextFormat == InsertTextFormat::Snippet) {
          if (!g_config->completion.placeholder) {
            // foo(${1:int a}, ${2:int b}) -> foo($1)$0
            auto p = s.find("${"), q = s.rfind('}');
            s.replace(p, q - p + 1, "$1");
          }
          s += "$0";
        }
        ls_items[j].priority_ = ccs->getPriority();
        if (!g_config->completion.detailedLabel) {
          ls_items[j].detail = ls_items[j].label;
          ls_items[j].label = ls_items[j].filterText;
        }
      }
      for (const FixItHint &fixIt : r.FixIts) {
        auto &ast = s.getASTContext();
        TextEdit ls_edit =
            ccls::toTextEdit(ast.getSourceManager(), ast.getLangOpts(), fixIt);
        for (size_t j = first_idx; j < ls_items.size(); j++)
          ls_items[j].additionalTextEdits.push_back(ls_edit);
      }
    }
  }

  CodeCompletionAllocator &getAllocator() override { return *alloc; }
  CodeCompletionTUInfo &getCodeCompletionTUInfo() override { return cctu_info; }
};
} // namespace

void MessageHandler::textDocument_completion(CompletionParam &param,
                                             ReplyOnce &reply) {
  static CompleteConsumerCache<std::vector<CompletionItem>> cache;
  std::string path = param.textDocument.uri.getPath();
  WorkingFile *wf = wfiles->getFile(path);
  if (!wf) {
    reply.notOpened(path);
    return;
  }

  CompletionList result;

  // It shouldn't be possible, but sometimes vscode will send queries out
  // of order, ie, we get completion request before buffer content update.
  std::string buffer_line;
  if (param.position.line >= 0 && param.position.line < wf->buffer_lines.size())
    buffer_line = wf->buffer_lines[param.position.line];

  clang::CodeCompleteOptions ccOpts;
  ccOpts.IncludeBriefComments = true;
  ccOpts.IncludeCodePatterns = StringRef(buffer_line).ltrim().startswith("#");
  ccOpts.IncludeFixIts = true;
  ccOpts.IncludeMacros = true;

  if (param.context.triggerKind == CompletionTriggerKind::TriggerCharacter &&
      param.context.triggerCharacter) {
    bool ok = true;
    int col = param.position.character - 2;
    switch ((*param.context.triggerCharacter)[0]) {
    case '"':
    case '/':
    case '<':
      ok = ccOpts.IncludeCodePatterns; // start with #
      break;
    case ':':
      ok = col >= 0 && buffer_line[col] == ':'; // ::
      break;
    case '>':
      ok = col >= 0 && buffer_line[col] == '-'; // ->
      break;
    }
    if (!ok) {
      reply(result);
      return;
    }
  }

  std::string filter;
  Position end_pos;
  Position begin_pos =
      wf->getCompletionPosition(param.position, &filter, &end_pos);

#if LLVM_VERSION_MAJOR < 8
  ParseIncludeLineResult preprocess = ParseIncludeLine(buffer_line);
  if (preprocess.ok && preprocess.keyword.compare("include") == 0) {
    CompletionList result;
    char quote = std::string(preprocess.match[5])[0];
    {
      std::unique_lock<std::mutex> lock(
          include_complete->completion_items_mutex, std::defer_lock);
      if (include_complete->is_scanning)
        lock.lock();
      for (auto &item : include_complete->completion_items)
        if (quote == '\0' || (item.quote_kind_ & 1 && quote == '"') ||
            (item.quote_kind_ & 2 && quote == '<'))
          result.items.push_back(item);
    }
    begin_pos.character = 0;
    end_pos.character = (int)buffer_line.size();
    filterCandidates(result, preprocess.pattern, begin_pos, end_pos,
                     buffer_line);
    decorateIncludePaths(preprocess.match, &result.items, quote);
    reply(result);
    return;
  }
#endif

  SemaManager::OnComplete callback =
      [filter, path, begin_pos, end_pos, reply,
       buffer_line](CodeCompleteConsumer *optConsumer) {
        if (!optConsumer)
          return;
        auto *consumer = static_cast<CompletionConsumer *>(optConsumer);
        CompletionList result;
        result.items = consumer->ls_items;

        filterCandidates(result, filter, begin_pos, end_pos, buffer_line);
        reply(result);
        if (!consumer->from_cache) {
          cache.withLock([&]() {
            cache.path = path;
            cache.line = buffer_line;
            cache.position = begin_pos;
            cache.result = consumer->ls_items;
          });
        }
      };

  if (cache.isCacheValid(path, buffer_line, begin_pos)) {
    CompletionConsumer consumer(ccOpts, true);
    cache.withLock([&]() { consumer.ls_items = cache.result; });
    callback(&consumer);
  } else {
    manager->comp_tasks.pushBack(std::make_unique<SemaManager::CompTask>(
        reply.id, param.textDocument.uri.getPath(), begin_pos,
        std::make_unique<CompletionConsumer>(ccOpts, false), ccOpts, callback));
  }
}
} // namespace ccls
