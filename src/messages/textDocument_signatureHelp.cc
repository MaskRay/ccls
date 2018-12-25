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

void BuildOptional(const CodeCompletionString &CCS, std::string &label,
                   std::vector<ParameterInformation> &ls_params) {
  for (const auto &Chunk : CCS) {
    switch (Chunk.Kind) {
    case CodeCompletionString::CK_Optional:
      BuildOptional(*Chunk.Optional, label, ls_params);
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
      label += Chunk.Text;
      ls_params.push_back({{off, (int)label.size()}});
      break;
    }
    case CodeCompletionString::CK_VerticalSpace:
      break;
    default:
      label += Chunk.Text;
      break;
    }
  }
}

class SignatureHelpConsumer : public CodeCompleteConsumer {
  std::shared_ptr<GlobalCodeCompletionAllocator> Alloc;
  CodeCompletionTUInfo CCTUInfo;
public:
  bool from_cache;
  SignatureHelp ls_sighelp;
  SignatureHelpConsumer(const clang::CodeCompleteOptions &CCOpts,
                        bool from_cache)
      : CodeCompleteConsumer(CCOpts, false),
        Alloc(std::make_shared<GlobalCodeCompletionAllocator>()),
        CCTUInfo(Alloc), from_cache(from_cache) {}
  void ProcessOverloadCandidates(Sema &S, unsigned CurrentArg,
                                 OverloadCandidate *Candidates,
                                 unsigned NumCandidates
#if LLVM_VERSION_MAJOR >= 8
                                 ,
                                 SourceLocation OpenParLoc
#endif
                                 ) override {
    ls_sighelp.activeParameter = (int)CurrentArg;
    for (unsigned i = 0; i < NumCandidates; i++) {
      OverloadCandidate Cand = Candidates[i];
      // We want to avoid showing instantiated signatures, because they may be
      // long in some cases (e.g. when 'T' is substituted with 'std::string', we
      // would get 'std::basic_string<char>').
      if (auto *Func = Cand.getFunction())
        if (auto *Pattern = Func->getTemplateInstantiationPattern())
          Cand = OverloadCandidate(Pattern);

      const auto *CCS =
          Cand.CreateSignatureString(CurrentArg, S, *Alloc, CCTUInfo, true);

      const char *ret_type = nullptr;
      SignatureInformation &ls_sig = ls_sighelp.signatures.emplace_back();
#if LLVM_VERSION_MAJOR >= 8
      const RawComment *RC = getCompletionComment(S.getASTContext(), Cand.getFunction());
      ls_sig.documentation = RC ? RC->getBriefText(S.getASTContext()) : "";
#endif
      for (const auto &Chunk : *CCS)
        switch (Chunk.Kind) {
        case CodeCompletionString::CK_ResultType:
          ret_type = Chunk.Text;
          break;
        case CodeCompletionString::CK_Placeholder:
        case CodeCompletionString::CK_CurrentParameter: {
          int off = (int)ls_sig.label.size();
          ls_sig.label += Chunk.Text;
          ls_sig.parameters.push_back({{off, (int)ls_sig.label.size()}});
          break;
        }
        case CodeCompletionString::CK_Optional:
          BuildOptional(*Chunk.Optional, ls_sig.label, ls_sig.parameters);
          break;
        case CodeCompletionString::CK_VerticalSpace:
          break;
        default:
          ls_sig.label += Chunk.Text;
          break;
        }
      if (ret_type) {
        ls_sig.label += " -> ";
        ls_sig.label += ret_type;
      }
    }
    std::sort(
        ls_sighelp.signatures.begin(), ls_sighelp.signatures.end(),
        [](const SignatureInformation &l, const SignatureInformation &r) {
          if (l.parameters.size() != r.parameters.size())
            return l.parameters.size() < r.parameters.size();
          if (l.label.size() != r.label.size())
            return l.label.size() < r.label.size();
          return l.label < r.label;
        });
  }

  CodeCompletionAllocator &getAllocator() override { return *Alloc; }
  CodeCompletionTUInfo &getCodeCompletionTUInfo() override { return CCTUInfo; }
};
} // namespace

void MessageHandler::textDocument_signatureHelp(
    TextDocumentPositionParam &param, ReplyOnce &reply) {
  static CompleteConsumerCache<SignatureHelp> cache;
  Position begin_pos = param.position;
  std::string path = param.textDocument.uri.GetPath();
  WorkingFile *wf = wfiles->GetFile(path);
  if (!wf) {
    reply.NotOpened(path);
    return;
  }
  {
    std::string filter;
    Position end_pos = param.position;
    begin_pos = wf->GetCompletionPosition(param.position, &filter, &end_pos);
  }

  SemaManager::OnComplete callback =
      [reply, path, begin_pos](CodeCompleteConsumer *OptConsumer) {
        if (!OptConsumer)
          return;
        auto *Consumer = static_cast<SignatureHelpConsumer *>(OptConsumer);
        reply(Consumer->ls_sighelp);
        if (!Consumer->from_cache) {
          cache.WithLock([&]() {
            cache.path = path;
            cache.position = begin_pos;
            cache.result = Consumer->ls_sighelp;
          });
        }
      };

  CodeCompleteOptions CCOpts;
  CCOpts.IncludeGlobals = false;
  CCOpts.IncludeMacros = false;
  CCOpts.IncludeBriefComments = false;
  if (cache.IsCacheValid(path, begin_pos)) {
    SignatureHelpConsumer Consumer(CCOpts, true);
    cache.WithLock([&]() { Consumer.ls_sighelp = cache.result; });
    callback(&Consumer);
  } else {
    manager->comp_tasks.PushBack(std::make_unique<SemaManager::CompTask>(
        reply.id, param.textDocument.uri.GetPath(), param.position,
        std::make_unique<SignatureHelpConsumer>(CCOpts, false), CCOpts,
        callback));
  }
}
} // namespace ccls
