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

#include "message_handler.hh"
#include "pipeline.hh"
#include "sema_manager.hh"

#include <clang/Sema/Sema.h>

namespace ccls {
using namespace clang;

namespace {
struct ParameterInformation {
  std::vector<int> label;
};
struct SignatureInformation {
  std::string label;
  std::optional<std::string> documentation;
  std::vector<ParameterInformation> parameters;
};
struct SignatureHelp {
  std::vector<SignatureInformation> signatures;
  int activeSignature = 0;
  int activeParameter = 0;
};
REFLECT_STRUCT(ParameterInformation, label);
REFLECT_STRUCT(SignatureInformation, label, documentation, parameters);
REFLECT_STRUCT(SignatureHelp, signatures, activeSignature, activeParameter);

void buildOptional(const CodeCompletionString &ccs, std::string &label,
                   std::vector<ParameterInformation> &ls_params) {
  for (const auto &chunk : ccs) {
    switch (chunk.Kind) {
    case CodeCompletionString::CK_Optional:
      buildOptional(*chunk.Optional, label, ls_params);
      break;
    case CodeCompletionString::CK_Placeholder:
      // A string that acts as a placeholder for, e.g., a function call
      // argument.
      // Intentional fallthrough here.
    case CodeCompletionString::CK_CurrentParameter: {
      // A piece of text that describes the parameter that corresponds to
      // the code-completion location within a function call, message send,
      // macro invocation, etc.
      int off = (int)label.size();
      label += chunk.Text;
      ls_params.push_back({{off, (int)label.size()}});
      break;
    }
    case CodeCompletionString::CK_VerticalSpace:
      break;
    default:
      label += chunk.Text;
      break;
    }
  }
}

class SignatureHelpConsumer : public CodeCompleteConsumer {
  std::shared_ptr<GlobalCodeCompletionAllocator> alloc;
  CodeCompletionTUInfo cCTUInfo;

public:
  bool from_cache;
  SignatureHelp ls_sighelp;
  SignatureHelpConsumer(const clang::CodeCompleteOptions &opts, bool from_cache)
      :
#if LLVM_VERSION_MAJOR >= 9 // rC358696
        CodeCompleteConsumer(opts),
#else
        CodeCompleteConsumer(opts, false),
#endif
        alloc(std::make_shared<GlobalCodeCompletionAllocator>()),
        cCTUInfo(alloc), from_cache(from_cache) {
  }
  void ProcessOverloadCandidates(Sema &s, unsigned currentArg,
                                 OverloadCandidate *candidates,
                                 unsigned numCandidates
#if LLVM_VERSION_MAJOR >= 8
                                 ,
                                 SourceLocation openParLoc
#endif
                                 ) override {
    ls_sighelp.activeParameter = (int)currentArg;
    for (unsigned i = 0; i < numCandidates; i++) {
      OverloadCandidate cand = candidates[i];
      // We want to avoid showing instantiated signatures, because they may be
      // long in some cases (e.g. when 'T' is substituted with 'std::string', we
      // would get 'std::basic_string<char>').
      if (auto *func = cand.getFunction())
        if (auto *pattern = func->getTemplateInstantiationPattern())
          cand = OverloadCandidate(pattern);

      const auto *ccs =
          cand.CreateSignatureString(currentArg, s, *alloc, cCTUInfo, true);

      const char *ret_type = nullptr;
      SignatureInformation &ls_sig = ls_sighelp.signatures.emplace_back();
      const RawComment *rc =
          getCompletionComment(s.getASTContext(), cand.getFunction());
      ls_sig.documentation = rc ? rc->getBriefText(s.getASTContext()) : "";
      for (const auto &chunk : *ccs)
        switch (chunk.Kind) {
        case CodeCompletionString::CK_ResultType:
          ret_type = chunk.Text;
          break;
        case CodeCompletionString::CK_Placeholder:
        case CodeCompletionString::CK_CurrentParameter: {
          int off = (int)ls_sig.label.size();
          ls_sig.label += chunk.Text;
          ls_sig.parameters.push_back({{off, (int)ls_sig.label.size()}});
          break;
        }
        case CodeCompletionString::CK_Optional:
          buildOptional(*chunk.Optional, ls_sig.label, ls_sig.parameters);
          break;
        case CodeCompletionString::CK_VerticalSpace:
          break;
        default:
          ls_sig.label += chunk.Text;
          break;
        }
      if (ret_type) {
        ls_sig.label += " -> ";
        ls_sig.label += ret_type;
      }
    }
    std::sort(ls_sighelp.signatures.begin(), ls_sighelp.signatures.end(),
              [](const SignatureInformation &l, const SignatureInformation &r) {
                if (l.parameters.size() != r.parameters.size())
                  return l.parameters.size() < r.parameters.size();
                if (l.label.size() != r.label.size())
                  return l.label.size() < r.label.size();
                return l.label < r.label;
              });
  }

  CodeCompletionAllocator &getAllocator() override { return *alloc; }
  CodeCompletionTUInfo &getCodeCompletionTUInfo() override { return cCTUInfo; }
};
} // namespace

void MessageHandler::textDocument_signatureHelp(
    TextDocumentPositionParam &param, ReplyOnce &reply) {
  static CompleteConsumerCache<SignatureHelp> cache;
  Position begin_pos = param.position;
  std::string path = param.textDocument.uri.getPath();
  WorkingFile *wf = wfiles->getFile(path);
  if (!wf) {
    reply.notOpened(path);
    return;
  }
  {
    std::string filter;
    Position end_pos;
    begin_pos = wf->getCompletionPosition(param.position, &filter, &end_pos);
  }

  SemaManager::OnComplete callback =
      [reply, path, begin_pos](CodeCompleteConsumer *optConsumer) {
        if (!optConsumer)
          return;
        auto *consumer = static_cast<SignatureHelpConsumer *>(optConsumer);
        reply(consumer->ls_sighelp);
        if (!consumer->from_cache) {
          cache.withLock([&]() {
            cache.path = path;
            cache.position = begin_pos;
            cache.result = consumer->ls_sighelp;
          });
        }
      };

  CodeCompleteOptions cCOpts;
  cCOpts.IncludeGlobals = false;
  cCOpts.IncludeMacros = false;
  cCOpts.IncludeBriefComments = true;
  if (cache.isCacheValid(path, begin_pos)) {
    SignatureHelpConsumer consumer(cCOpts, true);
    cache.withLock([&]() { consumer.ls_sighelp = cache.result; });
    callback(&consumer);
  } else {
    manager->comp_tasks.pushBack(std::make_unique<SemaManager::CompTask>(
        reply.id, param.textDocument.uri.getPath(), param.position,
        std::make_unique<SignatureHelpConsumer>(cCOpts, false), cCOpts,
        callback));
  }
}
} // namespace ccls
