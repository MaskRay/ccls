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
#include "message_handler.h"
#include "pipeline.hh"
using namespace ccls;

#include <clang/Sema/Sema.h>
using namespace clang;

#include <stdint.h>

namespace {
MethodType kMethodType = "textDocument/signatureHelp";

// Represents a parameter of a callable-signature. A parameter can
// have a label and a doc-comment.
struct lsParameterInformation {
  std::string label;
  // Not available in clang
  // std::optional<std::string> documentation;
};
MAKE_REFLECT_STRUCT(lsParameterInformation, label);

// Represents the signature of something callable. A signature
// can have a label, like a function-name, a doc-comment, and
// a set of parameters.
struct lsSignatureInformation {
  std::string label;
  std::optional<std::string> documentation;
  std::vector<lsParameterInformation> parameters;
};
MAKE_REFLECT_STRUCT(lsSignatureInformation, label, documentation, parameters);

// Signature help represents the signature of something
// callable. There can be multiple signature but only one
// active and only one active parameter.
struct lsSignatureHelp {
  std::vector<lsSignatureInformation> signatures;
  int activeSignature = 0;
  int activeParameter = 0;
};
MAKE_REFLECT_STRUCT(lsSignatureHelp, signatures, activeSignature,
                    activeParameter);

struct In_TextDocumentSignatureHelp : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentSignatureHelp, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentSignatureHelp);

struct Out_TextDocumentSignatureHelp
    : public lsOutMessage<Out_TextDocumentSignatureHelp> {
  lsRequestId id;
  lsSignatureHelp result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentSignatureHelp, jsonrpc, id, result);

std::string BuildOptional(const CodeCompletionString &CCS,
                          std::vector<lsParameterInformation> &ls_params) {
  std::string ret;
  for (const auto &Chunk : CCS) {
    switch (Chunk.Kind) {
    case CodeCompletionString::CK_Optional:
      ret += BuildOptional(*Chunk.Optional, ls_params);
      break;
    case CodeCompletionString::CK_Placeholder:
      // A string that acts as a placeholder for, e.g., a function call
      // argument.
      // Intentional fallthrough here.
    case CodeCompletionString::CK_CurrentParameter: {
      // A piece of text that describes the parameter that corresponds to
      // the code-completion location within a function call, message send,
      // macro invocation, etc.
      ret += Chunk.Text;
      ls_params.push_back(lsParameterInformation{Chunk.Text});
      break;
    }
    case CodeCompletionString::CK_VerticalSpace:
      break;
    default:
      ret += Chunk.Text;
      break;
    }
  }
  return ret;
}

class SignatureHelpConsumer : public CodeCompleteConsumer {
  std::shared_ptr<GlobalCodeCompletionAllocator> Alloc;
  CodeCompletionTUInfo CCTUInfo;
public:
  bool from_cache;
  lsSignatureHelp ls_sighelp;
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
      lsSignatureInformation &ls_sig = ls_sighelp.signatures.emplace_back();
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
          ls_sig.label += Chunk.Text;
          ls_sig.parameters.push_back(lsParameterInformation{Chunk.Text});
          break;
        }
        case CodeCompletionString::CK_Optional:
          ls_sig.label += BuildOptional(*Chunk.Optional, ls_sig.parameters);
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
        [](const lsSignatureInformation &l, const lsSignatureInformation &r) {
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

struct Handler_TextDocumentSignatureHelp
    : BaseMessageHandler<In_TextDocumentSignatureHelp> {
  MethodType GetMethodType() const override { return kMethodType; }

  void Run(In_TextDocumentSignatureHelp *request) override {
    static CompleteConsumerCache<lsSignatureHelp> cache;

    auto &params = request->params;
    std::string path = params.textDocument.uri.GetPath();
    if (WorkingFile *file = working_files->GetFileByFilename(path)) {
      std::string completion_text;
      lsPosition end_pos = params.position;
      params.position = file->FindStableCompletionSource(
        request->params.position, &completion_text, &end_pos);
    }

    CompletionManager::OnComplete callback =
        [id = request->id,
         params = request->params](CodeCompleteConsumer *OptConsumer) {
          if (!OptConsumer)
            return;
          auto *Consumer = static_cast<SignatureHelpConsumer *>(OptConsumer);
          Out_TextDocumentSignatureHelp out;
          out.id = id;
          out.result = Consumer->ls_sighelp;
          pipeline::WriteStdout(kMethodType, out);
          if (!Consumer->from_cache) {
            std::string path = params.textDocument.uri.GetPath();
            cache.WithLock([&]() {
              cache.path = path;
              cache.position = params.position;
              cache.result = Consumer->ls_sighelp;
            });
          }
        };

    CodeCompleteOptions CCOpts;
    CCOpts.IncludeGlobals = false;
    CCOpts.IncludeMacros = false;
    CCOpts.IncludeBriefComments = false;
    if (cache.IsCacheValid(params)) {
      SignatureHelpConsumer Consumer(CCOpts, true);
      cache.WithLock([&]() { Consumer.ls_sighelp = cache.result; });
      callback(&Consumer);
    } else {
      clang_complete->completion_request_.PushBack(
          std::make_unique<CompletionManager::CompletionRequest>(
              request->id, params.textDocument, params.position,
              std::make_unique<SignatureHelpConsumer>(CCOpts, false), CCOpts,
              callback));
    }
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentSignatureHelp);
} // namespace
