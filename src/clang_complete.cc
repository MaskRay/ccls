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

#include "clang_complete.h"

#include "clang_utils.h"
#include "filesystem.hh"
#include "log.hh"
#include "platform.h"

#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendDiagnostic.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Sema/CodeCompleteConsumer.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Config/llvm-config.h>
#include <llvm/Support/CrashRecoveryContext.h>
#include <llvm/Support/Threading.h>
using namespace clang;
using namespace llvm;

#include <algorithm>
#include <thread>

namespace ccls {
namespace {

std::string StripFileType(const std::string &path) {
  SmallString<128> Ret;
  sys::path::append(Ret, sys::path::parent_path(path), sys::path::stem(path));
  return Ret.str();
}

unsigned GetCompletionPriority(const CodeCompletionString &CCS,
                               CXCursorKind result_kind,
                               const std::optional<std::string> &typedText) {
  unsigned priority = CCS.getPriority();
  if (CCS.getAvailability() != CXAvailability_Available ||
      result_kind == CXCursor_Destructor ||
      result_kind == CXCursor_ConversionFunction ||
      (result_kind == CXCursor_CXXMethod && typedText &&
       StartsWith(*typedText, "operator")))
    priority *= 100;
  return priority;
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

void BuildCompletionItemTexts(std::vector<lsCompletionItem> &out,
                              CodeCompletionString &CCS,
                              bool include_snippets) {
  assert(!out.empty());
  auto out_first = out.size() - 1;

  std::string result_type;

  for (unsigned i = 0, num_chunks = CCS.size(); i < num_chunks; ++i) {
    const CodeCompletionString::Chunk &Chunk = CCS[i];
    CodeCompletionString::ChunkKind Kind = Chunk.Kind;
    std::string text;
    switch (Kind) {
    case CodeCompletionString::CK_TypedText:
    case CodeCompletionString::CK_Text:
    case CodeCompletionString::CK_Placeholder:
    case CodeCompletionString::CK_Informative:
      if (Chunk.Text)
        text = Chunk.Text;
      for (auto i = out_first; i < out.size(); i++) {
        // first TypedText is used for filtering
        if (Kind == CodeCompletionString::CK_TypedText && !out[i].filterText)
          out[i].filterText = text;
        if (Kind == CodeCompletionString::CK_Placeholder)
          out[i].parameters_.push_back(text);
      }
      break;
    case CodeCompletionString::CK_ResultType:
      if (Chunk.Text)
        result_type = Chunk.Text;
      continue;
    case CodeCompletionString::CK_CurrentParameter:
      // We have our own parsing logic for active parameter. This doesn't seem
      // to be very reliable.
      continue;
    case CodeCompletionString::CK_Optional: {
      // duplicate last element, the recursive call will complete it
      out.push_back(out.back());
      BuildCompletionItemTexts(out, *Chunk.Optional, include_snippets);
      continue;
    }
    // clang-format off
    case CodeCompletionString::CK_LeftParen: text = '('; break;
    case CodeCompletionString::CK_RightParen: text = ')'; break;
    case CodeCompletionString::CK_LeftBracket: text = '['; break;
    case CodeCompletionString::CK_RightBracket: text = ']'; break;
    case CodeCompletionString::CK_LeftBrace: text = '{'; break;
    case CodeCompletionString::CK_RightBrace: text = '}'; break;
    case CodeCompletionString::CK_LeftAngle: text = '<'; break;
    case CodeCompletionString::CK_RightAngle: text = '>'; break;
    case CodeCompletionString::CK_Comma: text = ", "; break;
    case CodeCompletionString::CK_Colon: text = ':'; break;
    case CodeCompletionString::CK_SemiColon: text = ';'; break;
    case CodeCompletionString::CK_Equal: text = '='; break;
    case CodeCompletionString::CK_HorizontalSpace: text = ' '; break;
    case CodeCompletionString::CK_VerticalSpace: text = ' '; break;
    // clang-format on
    }

    if (Kind != CodeCompletionString::CK_Informative)
      for (auto i = out_first; i < out.size(); ++i) {
        out[i].label += text;
        if (!include_snippets && !out[i].parameters_.empty())
          continue;

        if (Kind == CodeCompletionString::CK_Placeholder) {
          out[i].insertText += "${" +
                               std::to_string(out[i].parameters_.size()) + ":" +
                               text + "}";
          out[i].insertTextFormat = lsInsertTextFormat::Snippet;
        } else {
          out[i].insertText += text;
        }
      }
  }

  if (result_type.size())
    for (auto i = out_first; i < out.size(); ++i) {
      // ' : ' for variables,
      // ' -> ' (trailing return type-like) for functions
      out[i].label += (out[i].label == out[i].filterText ? " : " : " -> ");
      out[i].label += result_type;
    }
}

// |do_insert|: if |!do_insert|, do not append strings to |insert| after
// a placeholder.
void BuildDetailString(const CodeCompletionString &CCS, lsCompletionItem &item,
                       bool &do_insert, std::vector<std::string> *parameters,
                       bool include_snippets, int &angle_stack) {
  for (unsigned i = 0, num_chunks = CCS.size(); i < num_chunks; ++i) {
    const CodeCompletionString::Chunk &Chunk = CCS[i];
    CodeCompletionString::ChunkKind Kind = Chunk.Kind;
    const char *text = nullptr;
    switch (Kind) {
    case CodeCompletionString::CK_TypedText:
      item.label = Chunk.Text;
      [[fallthrough]];
    case CodeCompletionString::CK_Text:
      item.detail += Chunk.Text;
      if (do_insert)
        item.insertText += Chunk.Text;
      break;
    case CodeCompletionString::CK_Placeholder: {
      parameters->push_back(Chunk.Text);
      item.detail += Chunk.Text;
      // Add parameter declarations as snippets if enabled
      if (include_snippets) {
        item.insertText +=
            "${" + std::to_string(parameters->size()) + ":" + Chunk.Text + "}";
        item.insertTextFormat = lsInsertTextFormat::Snippet;
      } else
        do_insert = false;
      break;
    }
    case CodeCompletionString::CK_Informative:
      item.detail += Chunk.Text;
      break;
    case CodeCompletionString::CK_Optional: {
      // Do not add text to insert string if we're in angle brackets.
      bool should_insert = do_insert && angle_stack == 0;
      BuildDetailString(*Chunk.Optional, item, should_insert, parameters,
                        include_snippets, angle_stack);
      break;
    }
    case CodeCompletionString::CK_ResultType:
      item.detail = Chunk.Text + item.detail + " ";
      break;
    case CodeCompletionString::CK_CurrentParameter:
      // We have our own parsing logic for active parameter. This doesn't seem
      // to be very reliable.
      break;
    // clang-format off
    case CodeCompletionString::CK_LeftParen: text = "("; break;
    case CodeCompletionString::CK_RightParen: text = ")"; break;
    case CodeCompletionString::CK_LeftBracket: text = "["; break;
    case CodeCompletionString::CK_RightBracket: text = "]"; break;
    case CodeCompletionString::CK_LeftBrace: text = "{"; break;
    case CodeCompletionString::CK_RightBrace: text = "}"; break;
    case CodeCompletionString::CK_LeftAngle: text = "<"; angle_stack++; break;
    case CodeCompletionString::CK_RightAngle: text = ">"; angle_stack--; break;
    case CodeCompletionString::CK_Comma: text = ", "; break;
    case CodeCompletionString::CK_Colon: text = ":"; break;
    case CodeCompletionString::CK_SemiColon: text = ";"; break;
    case CodeCompletionString::CK_Equal: text = "="; break;
    case CodeCompletionString::CK_HorizontalSpace:
    case CodeCompletionString::CK_VerticalSpace: text = " "; break;
    // clang-format on
    }
    if (text) {
      item.detail += text;
      if (do_insert && include_snippets)
        item.insertText += text;
    }
  }
}

bool LocationInRange(SourceLocation L, CharSourceRange R,
                     const SourceManager &M) {
  assert(R.isCharRange());
  if (!R.isValid() || M.getFileID(R.getBegin()) != M.getFileID(R.getEnd()) ||
      M.getFileID(R.getBegin()) != M.getFileID(L))
    return false;
  return L != R.getEnd() && M.isPointWithin(L, R.getBegin(), R.getEnd());
}

CharSourceRange DiagnosticRange(const clang::Diagnostic &D, const LangOptions &L) {
  auto &M = D.getSourceManager();
  auto Loc = M.getFileLoc(D.getLocation());
  // Accept the first range that contains the location.
  for (const auto &CR : D.getRanges()) {
    auto R = Lexer::makeFileCharRange(CR, M, L);
    if (LocationInRange(Loc, R, M))
      return R;
  }
  // The range may be given as a fixit hint instead.
  for (const auto &F : D.getFixItHints()) {
    auto R = Lexer::makeFileCharRange(F.RemoveRange, M, L);
    if (LocationInRange(Loc, R, M))
      return R;
  }
  // If no suitable range is found, just use the token at the location.
  auto R = Lexer::makeFileCharRange(CharSourceRange::getTokenRange(Loc), M, L);
  if (!R.isValid()) // Fall back to location only, let the editor deal with it.
    R = CharSourceRange::getCharRange(Loc);
  return R;
}


class CaptureCompletionResults : public CodeCompleteConsumer {
  std::shared_ptr<clang::GlobalCodeCompletionAllocator> Alloc;
  CodeCompletionTUInfo CCTUInfo;

public:
  std::vector<lsCompletionItem> ls_items;

  CaptureCompletionResults(const CodeCompleteOptions &Opts)
      : CodeCompleteConsumer(Opts, false),
        Alloc(std::make_shared<clang::GlobalCodeCompletionAllocator>()),
        CCTUInfo(Alloc) {}

  void ProcessCodeCompleteResults(Sema &S, CodeCompletionContext Context,
                                  CodeCompletionResult *Results,
                                  unsigned NumResults) override {
    ls_items.reserve(NumResults);
    for (unsigned i = 0; i != NumResults; i++) {
      if (Results[i].Availability == CXAvailability_NotAccessible ||
          Results[i].Availability == CXAvailability_NotAvailable)
        continue;
      CodeCompletionString *CCS = Results[i].CreateCodeCompletionString(
          S, Context, getAllocator(), getCodeCompletionTUInfo(),
          includeBriefComments());
      lsCompletionItem ls_item;
      ls_item.kind = GetCompletionKind(Results[i].CursorKind);
      if (const char *brief = CCS->getBriefComment())
        ls_item.documentation = brief;

      // label/detail/filterText/insertText/priority
      if (g_config->completion.detailedLabel) {
        ls_item.detail = CCS->getParentContextName().str();

        size_t first_idx = ls_items.size();
        ls_items.push_back(ls_item);
        BuildCompletionItemTexts(ls_items, *CCS,
                                 g_config->client.snippetSupport);

        for (size_t j = first_idx; j < ls_items.size(); j++) {
          if (g_config->client.snippetSupport &&
              ls_items[j].insertTextFormat == lsInsertTextFormat::Snippet)
            ls_items[j].insertText += "$0";
          ls_items[j].priority_ = GetCompletionPriority(
              *CCS, Results[i].CursorKind, ls_items[j].filterText);
        }
      } else {
        bool do_insert = true;
        int angle_stack = 0;
        BuildDetailString(*CCS, ls_item, do_insert, &ls_item.parameters_,
                          g_config->client.snippetSupport, angle_stack);
        if (g_config->client.snippetSupport &&
            ls_item.insertTextFormat == lsInsertTextFormat::Snippet)
          ls_item.insertText += "$0";
        ls_item.priority_ =
            GetCompletionPriority(*CCS, Results[i].CursorKind, ls_item.label);
        ls_items.push_back(ls_item);
      }
    }
  }

  void ProcessOverloadCandidates(Sema &S, unsigned CurrentArg,
                                 OverloadCandidate *Candidates,
                                 unsigned NumCandidates) override {}

  CodeCompletionAllocator &getAllocator() override { return *Alloc; }

  CodeCompletionTUInfo &getCodeCompletionTUInfo() override { return CCTUInfo; }
};

class StoreDiags : public DiagnosticConsumer {
  const LangOptions *LangOpts;
  std::optional<Diag> last;
  std::vector<Diag> output;
  void Flush() {
    if (!last)
      return;
    bool mentions = last->inside_main || last->edits.size();
    if (!mentions)
      for (auto &N : last->notes)
        if (N.inside_main)
          mentions = true;
    if (mentions)
      output.push_back(std::move(*last));
    last.reset();
  }
public:
  std::vector<Diag> Take() {
    return std::move(output);
  }
  void BeginSourceFile(const LangOptions &Opts, const Preprocessor *) override {
    LangOpts = &Opts;
  }
  void EndSourceFile() override {
    Flush();
  }
  void HandleDiagnostic(DiagnosticsEngine::Level Level,
                        const Diagnostic &Info) override {
    DiagnosticConsumer::HandleDiagnostic(Level, Info);
    SourceLocation L = Info.getLocation();
    if (!L.isValid()) return;
    const SourceManager &SM = Info.getSourceManager();
    bool inside_main = SM.isInMainFile(L);
    auto fillDiagBase = [&](DiagBase &d) {
      llvm::SmallString<64> Message;
      Info.FormatDiagnostic(Message);
      d.range =
          FromCharSourceRange(SM, *LangOpts, DiagnosticRange(Info, *LangOpts));
      d.message = Message.str();
      d.inside_main = inside_main;
      d.file = SM.getFilename(Info.getLocation());
      d.level = Level;
      d.category = DiagnosticIDs::getCategoryNumberForDiag(Info.getID());
    };

    auto addFix = [&](bool SyntheticMessage) -> bool {
      if (!inside_main)
        return false;
      for (const FixItHint &FixIt : Info.getFixItHints()) {
        if (!SM.isInMainFile(FixIt.RemoveRange.getBegin()))
          return false;
        lsTextEdit edit;
        edit.newText = FixIt.CodeToInsert;
        auto r = FromCharSourceRange(SM, *LangOpts, FixIt.RemoveRange);
        edit.range =
            lsRange{{r.start.line, r.start.column}, {r.end.line, r.end.column}};
        last->edits.push_back(std::move(edit));
      }
      return true;
    };

    if (Level == DiagnosticsEngine::Note || Level == DiagnosticsEngine::Remark) {
      if (Info.getFixItHints().size()) {
        addFix(false);
      } else {
        Note &n = last->notes.emplace_back();
        fillDiagBase(n);
      }
    } else {
      Flush();
      last = Diag();
      fillDiagBase(*last);
      if (!Info.getFixItHints().empty())
        addFix(true);
    }
  }
};

std::unique_ptr<CompilerInvocation>
buildCompilerInvocation(const std::vector<std::string> &args,
                        IntrusiveRefCntPtr<vfs::FileSystem> VFS) {
  std::vector<const char *> cargs;
  for (auto &arg : args)
    cargs.push_back(arg.c_str());
  IntrusiveRefCntPtr<DiagnosticsEngine> Diags(
      CompilerInstance::createDiagnostics(new DiagnosticOptions));
  std::unique_ptr<CompilerInvocation> CI =
      createInvocationFromCommandLine(cargs, Diags, VFS);
  if (CI) {
    CI->getFrontendOpts().DisableFree = false;
    CI->getLangOpts()->CommentOpts.ParseAllComments = true;
    CI->getLangOpts()->SpellChecking = false;
  }
  return CI;
}

std::unique_ptr<CompilerInstance>
BuildCompilerInstance(CompletionSession &session,
                      std::unique_ptr<CompilerInvocation> CI,
  DiagnosticConsumer &DC,
                      const WorkingFiles::Snapshot &snapshot,
                      std::vector<std::unique_ptr<llvm::MemoryBuffer>> &Bufs) {
  for (auto &file : snapshot.files) {
    Bufs.push_back(llvm::MemoryBuffer::getMemBuffer(file.content));
    if (file.filename == session.file.filename) {
      if (auto Preamble = session.GetPreamble()) {
#if LLVM_VERSION_MAJOR >= 7
        Preamble->Preamble.OverridePreamble(*CI, session.FS,
                                            Bufs.back().get());
#else
        Preamble->Preamble.AddImplicitPreamble(*CI, session.FS,
                                               Bufs.back().get());
#endif
      } else {
        CI->getPreprocessorOpts().addRemappedFile(
            CI->getFrontendOpts().Inputs[0].getFile(), Bufs.back().get());
      }
    } else {
      CI->getPreprocessorOpts().addRemappedFile(file.filename,
                                                Bufs.back().get());
    }
  }

  auto Clang = std::make_unique<CompilerInstance>(session.PCH);
  Clang->setInvocation(std::move(CI));
  Clang->setVirtualFileSystem(session.FS);
  Clang->createDiagnostics(&DC, false);
  Clang->setTarget(TargetInfo::CreateTargetInfo(
      Clang->getDiagnostics(), Clang->getInvocation().TargetOpts));
  if (!Clang->hasTarget())
    return nullptr;
  return Clang;
}

bool Parse(CompilerInstance &Clang) {
  SyntaxOnlyAction Action;
  if (!Action.BeginSourceFile(Clang, Clang.getFrontendOpts().Inputs[0]))
    return false;
  if (!Action.Execute())
    return false;
  Action.EndSourceFile();
  return true;
}

void CompletionPreloadMain(ClangCompleteManager *completion_manager) {
  while (true) {
    // Fetching the completion request blocks until we have a request.
    auto request = completion_manager->preload_requests_.Dequeue();

    // If we don't get a session then that means we don't care about the file
    // anymore - abandon the request.
    std::shared_ptr<CompletionSession> session =
        completion_manager->TryGetSession(request.path,
                                          false /*mark_as_completion*/,
                                          false /*create_if_needed*/);
    if (!session)
      continue;

    const auto &args = session->file.args;
    WorkingFiles::Snapshot snapshot = session->wfiles->AsSnapshot(
      {StripFileType(session->file.filename)});

    LOG_S(INFO) << "create completion session for " << session->file.filename;
    if (std::unique_ptr<CompilerInvocation> CI =
            buildCompilerInvocation(args, session->FS))
      session->BuildPreamble(*CI);
  }
}

void CompletionQueryMain(ClangCompleteManager *completion_manager) {
  while (true) {
    // Fetching the completion request blocks until we have a request.
    std::unique_ptr<ClangCompleteManager::CompletionRequest> request =
        completion_manager->completion_request_.Dequeue();

    // Drop older requests if we're not buffering.
    while (g_config->completion.dropOldRequests &&
           !completion_manager->completion_request_.IsEmpty()) {
      completion_manager->on_dropped_(request->id);
      request = completion_manager->completion_request_.Dequeue();
    }

    std::string path = request->document.uri.GetPath();

    std::shared_ptr<CompletionSession> session =
        completion_manager->TryGetSession(path, true /*mark_as_completion*/,
                                          true /*create_if_needed*/);

    std::unique_ptr<CompilerInvocation> CI =
        buildCompilerInvocation(session->file.args, session->FS);
    if (!CI)
      continue;
    clang::CodeCompleteOptions CCOpts;
#if LLVM_VERSION_MAJOR >= 7
    CCOpts.IncludeFixIts = true;
#endif
    CCOpts.IncludeCodePatterns = true;
    auto &FOpts = CI->getFrontendOpts();
    FOpts.CodeCompleteOpts = CCOpts;
    FOpts.CodeCompletionAt.FileName = session->file.filename;
    FOpts.CodeCompletionAt.Line = request->position.line + 1;
    FOpts.CodeCompletionAt.Column = request->position.character + 1;

    StoreDiags DC;
    WorkingFiles::Snapshot snapshot =
      completion_manager->working_files_->AsSnapshot({StripFileType(path)});
    std::vector<std::unique_ptr<llvm::MemoryBuffer>> Bufs;
    auto Clang = BuildCompilerInstance(*session, std::move(CI), DC, snapshot, Bufs);
    if (!Clang)
      continue;

    auto Consumer = new CaptureCompletionResults(CCOpts);
    Clang->setCodeCompletionConsumer(Consumer);
    if (!Parse(*Clang))
      continue;
    for (auto &Buf : Bufs)
      Buf.release();

    request->on_complete(Consumer->ls_items, false /*is_cached_result*/);
  }
}

void DiagnosticQueryMain(ClangCompleteManager *manager) {
  while (true) {
    // Fetching the completion request blocks until we have a request.
    ClangCompleteManager::DiagnosticRequest request =
        manager->diagnostic_request_.Dequeue();
    if (!g_config->diagnostics.onType)
      continue;
    std::string path = request.document.uri.GetPath();

    std::shared_ptr<CompletionSession> session = manager->TryGetSession(
        path, true /*mark_as_completion*/, true /*create_if_needed*/);

    std::unique_ptr<CompilerInvocation> CI =
        buildCompilerInvocation(session->file.args, session->FS);
    if (!CI)
      continue;
    StoreDiags DC;
    WorkingFiles::Snapshot snapshot =
        manager->working_files_->AsSnapshot({StripFileType(path)});
    std::vector<std::unique_ptr<llvm::MemoryBuffer>> Bufs;
    auto Clang = BuildCompilerInstance(*session, std::move(CI), DC, snapshot, Bufs);
    if (!Clang)
      continue;
    if (!Parse(*Clang))
      continue;
    for (auto &Buf : Bufs)
      Buf.release();

    std::vector<lsDiagnostic> ls_diags;
    for (auto &d : DC.Take()) {
      if (!d.inside_main)
        continue;
      lsDiagnostic &ls_diag = ls_diags.emplace_back();
      ls_diag.range = lsRange{{d.range.start.line, d.range.start.column},
                              {d.range.end.line, d.range.end.column}};
      ls_diag.message = d.message;
      switch (d.level) {
      case DiagnosticsEngine::Ignored:
        // llvm_unreachable
      case DiagnosticsEngine::Note:
      case DiagnosticsEngine::Remark:
        ls_diag.severity = lsDiagnosticSeverity::Information;
        continue;
      case DiagnosticsEngine::Warning:
        ls_diag.severity = lsDiagnosticSeverity::Warning;
        break;
      case DiagnosticsEngine::Error:
      case DiagnosticsEngine::Fatal:
        ls_diag.severity = lsDiagnosticSeverity::Error;
      }
      ls_diag.code = d.category;
      ls_diag.fixits_ = d.edits;
    }
    manager->on_diagnostic_(path, ls_diags);
  }
}

} // namespace

std::shared_ptr<PreambleData> CompletionSession::GetPreamble() {
  std::lock_guard<std::mutex> lock(mutex);
  return preamble;
}

void CompletionSession::BuildPreamble(CompilerInvocation &CI) {
  std::shared_ptr<PreambleData> OldP = GetPreamble();
  std::string content = wfiles->GetContent(file.filename);
  std::unique_ptr<llvm::MemoryBuffer> Buf =
      llvm::MemoryBuffer::getMemBuffer(content);
  auto Bounds = ComputePreambleBounds(*CI.getLangOpts(), Buf.get(), 0);
  if (OldP && OldP->Preamble.CanReuse(CI, Buf.get(), Bounds, FS.get()))
    return;
  CI.getFrontendOpts().SkipFunctionBodies = true;
#if LLVM_VERSION_MAJOR >= 7
  CI.getPreprocessorOpts().WriteCommentListToPCH = false;
#endif

  StoreDiags DC;
  IntrusiveRefCntPtr<DiagnosticsEngine> DE =
      CompilerInstance::createDiagnostics(&CI.getDiagnosticOpts(), &DC, false);
  PreambleCallbacks PP;
  if (auto NewPreamble = PrecompiledPreamble::Build(CI, Buf.get(), Bounds,
      *DE, FS, PCH, true, PP)) {
    std::lock_guard<std::mutex> lock(mutex);
    preamble =
        std::make_shared<PreambleData>(std::move(*NewPreamble), DC.Take());
  }
}

} // namespace ccls

ClangCompleteManager::ClangCompleteManager(Project *project,
                                           WorkingFiles *working_files,
                                           OnDiagnostic on_diagnostic,
                                           OnDropped on_dropped)
    : project_(project), working_files_(working_files),
      on_diagnostic_(on_diagnostic), on_dropped_(on_dropped),
      preloaded_sessions_(kMaxPreloadedSessions),
      completion_sessions_(kMaxCompletionSessions),
      PCH(std::make_shared<PCHContainerOperations>()) {
  std::thread([&]() {
    set_thread_name("comp-query");
    ccls::CompletionQueryMain(this);
  })
      .detach();
  std::thread([&]() {
    set_thread_name("comp-preload");
    ccls::CompletionPreloadMain(this);
  })
      .detach();
  std::thread([&]() {
    set_thread_name("diag-query");
    ccls::DiagnosticQueryMain(this);
  })
      .detach();
}

void ClangCompleteManager::CodeComplete(
    const lsRequestId &id,
    const lsTextDocumentPositionParams &completion_location,
    const OnComplete &on_complete) {
  completion_request_.PushBack(std::make_unique<CompletionRequest>(
      id, completion_location.textDocument, completion_location.position,
      on_complete));
}

void ClangCompleteManager::DiagnosticsUpdate(
    const lsTextDocumentIdentifier &document) {
  bool has = false;
  diagnostic_request_.Iterate([&](const DiagnosticRequest &request) {
    if (request.document.uri == document.uri)
      has = true;
  });
  if (!has)
    diagnostic_request_.PushBack(DiagnosticRequest{document},
                                 true /*priority*/);
}

void ClangCompleteManager::NotifyView(const std::string &filename) {
  //
  // On view, we reparse only if the file has not been parsed. The existence of
  // a CompletionSession instance implies the file is already parsed or will be
  // parsed soon.
  //

  // Only reparse the file if we create a new CompletionSession.
  if (EnsureCompletionOrCreatePreloadSession(filename))
    preload_requests_.PushBack(PreloadRequest(filename), true);
}

void ClangCompleteManager::NotifyEdit(const std::string &filename) {
  //
  // We treat an edit like a view, because the completion logic will handle
  // moving the CompletionSession instance from preloaded to completion
  // storage.
  //

  NotifyView(filename);
}

void ClangCompleteManager::NotifySave(const std::string &filename) {
  //
  // On save, always reparse.
  //

  EnsureCompletionOrCreatePreloadSession(filename);
  preload_requests_.PushBack(PreloadRequest(filename), true);
}

void ClangCompleteManager::NotifyClose(const std::string &filename) {
  //
  // On close, we clear any existing CompletionSession instance.
  //

  std::lock_guard<std::mutex> lock(sessions_lock_);

  // Take and drop. It's okay if we don't actually drop the file, it'll
  // eventually get pushed out of the caches as the user opens other files.
  auto preloaded_ptr = preloaded_sessions_.TryTake(filename);
  LOG_IF_S(INFO, !!preloaded_ptr)
      << "Dropped preloaded-based code completion session for " << filename;
  auto completion_ptr = completion_sessions_.TryTake(filename);
  LOG_IF_S(INFO, !!completion_ptr)
      << "Dropped completion-based code completion session for " << filename;

  // We should never have both a preloaded and completion session.
  assert((preloaded_ptr && completion_ptr) == false);
}

bool ClangCompleteManager::EnsureCompletionOrCreatePreloadSession(
    const std::string &filename) {
  std::lock_guard<std::mutex> lock(sessions_lock_);

  // Check for an existing CompletionSession.
  if (preloaded_sessions_.TryGet(filename) ||
      completion_sessions_.TryGet(filename)) {
    return false;
  }

  // No CompletionSession, create new one.
  auto session = std::make_shared<ccls::CompletionSession>(
      project_->FindCompilationEntryForFile(filename), working_files_, PCH);
  preloaded_sessions_.Insert(session->file.filename, session);
  return true;
}

std::shared_ptr<ccls::CompletionSession>
ClangCompleteManager::TryGetSession(const std::string &filename,
                                    bool mark_as_completion,
                                    bool create_if_needed) {
  std::lock_guard<std::mutex> lock(sessions_lock_);

  // Try to find a preloaded session.
  std::shared_ptr<ccls::CompletionSession> preloaded =
      preloaded_sessions_.TryGet(filename);

  if (preloaded) {
    // If this request is for a completion, we should move it to
    // |completion_sessions|.
    if (mark_as_completion) {
      preloaded_sessions_.TryTake(filename);
      completion_sessions_.Insert(filename, preloaded);
    }
    return preloaded;
  }

  // Try to find a completion session. If none create one.
  std::shared_ptr<ccls::CompletionSession> session =
      completion_sessions_.TryGet(filename);
  if (!session && create_if_needed) {
    session = std::make_shared<ccls::CompletionSession>(
        project_->FindCompilationEntryForFile(filename), working_files_, PCH);
    completion_sessions_.Insert(filename, session);
  }

  return session;
}

void ClangCompleteManager::FlushSession(const std::string &filename) {
  std::lock_guard<std::mutex> lock(sessions_lock_);

  preloaded_sessions_.TryTake(filename);
  completion_sessions_.TryTake(filename);
}

void ClangCompleteManager::FlushAllSessions() {
  LOG_S(INFO) << "flush all clang complete sessions";
  std::lock_guard<std::mutex> lock(sessions_lock_);

  preloaded_sessions_.Clear();
  completion_sessions_.Clear();
}

void CodeCompleteCache::WithLock(std::function<void()> action) {
  std::lock_guard<std::mutex> lock(mutex_);
  action();
}

bool CodeCompleteCache::IsCacheValid(lsTextDocumentPositionParams position) {
  std::lock_guard<std::mutex> lock(mutex_);
  return cached_path_ == position.textDocument.uri.GetPath() &&
         cached_completion_position_ == position.position;
}
