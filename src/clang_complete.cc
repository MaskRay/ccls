#include "clang_complete.h"

#include "clang_utils.h"
#include "filesystem.hh"
#include "log.hh"
#include "platform.h"

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include <clang/Sema/CodeCompleteConsumer.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/Threading.h>
using namespace clang;
using namespace llvm;

#include <algorithm>
#include <thread>

namespace {

std::string StripFileType(const std::string& path) {
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
          out[i].insertText +=
            "${" + std::to_string(out[i].parameters_.size()) + ":" + text + "}";
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
    const char* text = nullptr;
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
        item.insertText += "${" + std::to_string(parameters->size()) + ":" + Chunk.Text + "}";
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
      BuildDetailString(*Chunk.Optional, item, should_insert,
                        parameters, include_snippets, angle_stack);
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

class CaptureCompletionResults : public CodeCompleteConsumer {
  std::shared_ptr<clang::GlobalCodeCompletionAllocator> Alloc;
  CodeCompletionTUInfo CCTUInfo;

public:
  std::vector<lsCompletionItem> ls_items;

  CaptureCompletionResults(const CodeCompleteOptions &Opts)
      : CodeCompleteConsumer(Opts, false),
        Alloc(std::make_shared<clang::GlobalCodeCompletionAllocator>()),
        CCTUInfo(Alloc) {}

  void ProcessCodeCompleteResults(Sema &S, 
    CodeCompletionContext Context,
    CodeCompletionResult *Results,
    unsigned NumResults) override {
    ls_items.reserve(NumResults);
    for (unsigned i = 0; i != NumResults; i++) {
      CodeCompletionString *CCS = Results[i].CreateCodeCompletionString(
          S, Context, getAllocator(), getCodeCompletionTUInfo(),
          includeBriefComments());
      if (CCS->getAvailability() == CXAvailability_NotAvailable)
        continue;

      lsCompletionItem ls_item;
      ls_item.kind = GetCompletionKind(Results[i].CursorKind);
      if (const char* brief = CCS->getBriefComment())
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
              *CCS, Results[i].CursorKind, ls_items[i].filterText);
        }
      } else {
        bool do_insert = true;
        int angle_stack = 0;
        BuildDetailString(*CCS, ls_item, do_insert,
                          &ls_item.parameters_,
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

  CodeCompletionAllocator &getAllocator() override { return *Alloc; }

  CodeCompletionTUInfo &getCodeCompletionTUInfo() override { return CCTUInfo;}
};

void TryEnsureDocumentParsed(ClangCompleteManager* manager,
                             std::shared_ptr<CompletionSession> session,
                             std::unique_ptr<ClangTranslationUnit>* tu,
                             bool emit_diag) {
  // Nothing to do. We already have a translation unit.
  if (*tu)
    return;

  const auto &args = session->file.args;
  WorkingFiles::Snapshot snapshot = session->working_files->AsSnapshot(
      {StripFileType(session->file.filename)});

  LOG_S(INFO) << "Creating completion session with arguments "
              << StringJoin(args, " ");
  *tu = ClangTranslationUnit::Create(session->file.filename, args, snapshot);
}

void CompletionPreloadMain(ClangCompleteManager* completion_manager) {
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

    // Note: we only preload completion. We emit diagnostics for the
    // completion preload though.
    CompletionSession::Tu* tu = &session->completion;

    // If we've parsed it more recently than the request time, don't bother
    // reparsing.
    if (tu->last_parsed_at && *tu->last_parsed_at > request.request_time)
      continue;

    std::unique_ptr<ClangTranslationUnit> parsing;
    TryEnsureDocumentParsed(completion_manager, session, &parsing, true);

    // Activate new translation unit.
    std::lock_guard<std::mutex> lock(tu->lock);
    tu->last_parsed_at = std::chrono::high_resolution_clock::now();
    tu->tu = std::move(parsing);
  }
}

void CompletionQueryMain(ClangCompleteManager* completion_manager) {
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

    std::lock_guard<std::mutex> lock(session->completion.lock);
    TryEnsureDocumentParsed(completion_manager, session, &session->completion.tu, false);

    // It is possible we failed to create the document despite
    // |TryEnsureDocumentParsed|.
    if (ClangTranslationUnit* tu = session->completion.tu.get()) {
      WorkingFiles::Snapshot snapshot =
          completion_manager->working_files_->AsSnapshot({StripFileType(path)});
      IntrusiveRefCntPtr<FileManager> FileMgr(&tu->Unit->getFileManager());
      IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts(new DiagnosticOptions());
      IntrusiveRefCntPtr<DiagnosticsEngine> Diag(new DiagnosticsEngine(
          IntrusiveRefCntPtr<DiagnosticIDs>(new DiagnosticIDs), &*DiagOpts));
      // StoreDiags Diags;
      // IntrusiveRefCntPtr<DiagnosticsEngine> DiagE =
      //     CompilerInstance::createDiagnostics(DiagOpts.get(), &Diags, false);

      IntrusiveRefCntPtr<SourceManager> SrcMgr(
          new SourceManager(*Diag, *FileMgr));
      std::vector<ASTUnit::RemappedFile> Remapped = GetRemapped(snapshot);
      SmallVector<StoredDiagnostic, 8> Diagnostics;
      SmallVector<const llvm::MemoryBuffer *, 1> TemporaryBuffers;

      CodeCompleteOptions Opts;
      LangOptions LangOpts;
      Opts.IncludeBriefComments = true;
      Opts.LoadExternal = false;
      Opts.IncludeFixIts = true;
      CaptureCompletionResults capture(Opts);
      tu->Unit->CodeComplete(session->file.filename, request->position.line + 1,
                             request->position.character + 1, Remapped,
                             /*IncludeMacros=*/true,
                             /*IncludeCodePatterns=*/false,
                             /*IncludeBriefComments=*/true, capture, tu->PCHCO,
                             *Diag, LangOpts, *SrcMgr, *FileMgr, Diagnostics,
                             TemporaryBuffers);
      request->on_complete(capture.ls_items, false /*is_cached_result*/);
      // completion_manager->on_diagnostic_(session->file.filename, Diags.take());
    }
  }
}

void DiagnosticQueryMain(ClangCompleteManager* completion_manager) {
  while (true) {
    // Fetching the completion request blocks until we have a request.
    ClangCompleteManager::DiagnosticRequest request =
        completion_manager->diagnostic_request_.Dequeue();
    if (!g_config->diagnostics.onType)
      continue;
    std::string path = request.document.uri.GetPath();

    std::shared_ptr<CompletionSession> session =
        completion_manager->TryGetSession(path, true /*mark_as_completion*/,
                                          true /*create_if_needed*/);

    // At this point, we must have a translation unit. Block until we have one.
    std::lock_guard<std::mutex> lock(session->diagnostics.lock);
    TryEnsureDocumentParsed(completion_manager, session,
                            &session->diagnostics.tu,
                            false /*emit_diagnostics*/);

    // It is possible we failed to create the document despite
    // |TryEnsureDocumentParsed|.
    ClangTranslationUnit* tu = session->diagnostics.tu.get();
    if (!tu)
      continue;

    WorkingFiles::Snapshot snapshot =
        completion_manager->working_files_->AsSnapshot({StripFileType(path)});
    llvm::CrashRecoveryContext CRC;
    if (tu->Reparse(CRC, snapshot)) {
      LOG_S(ERROR) << "Reparsing translation unit for diagnostics failed for "
                   << path;
      continue;
    }

    auto &SM = tu->Unit->getSourceManager();
    auto &LangOpts = tu->Unit->getLangOpts();
    std::vector<lsDiagnostic> ls_diags;
    for (ASTUnit::stored_diag_iterator I = tu->Unit->stored_diag_begin(),
                                       E = tu->Unit->stored_diag_end();
         I != E; ++I) {
      FullSourceLoc FLoc = I->getLocation();
      SourceRange R;
      for (const auto &CR : I->getRanges()) {
        auto RT = Lexer::makeFileCharRange(CR, SM, LangOpts);
        if (SM.isPointWithin(FLoc, RT.getBegin(), RT.getEnd())) {
          R = CR.getAsRange();
          break;
        }
      }
      Range r = R.isValid() ? FromCharRange(SM, LangOpts, R)
                            : FromTokenRange(SM, LangOpts, {FLoc, FLoc});
      lsDiagnostic ls_diag;
      ls_diag.range =
          lsRange{{r.start.line, r.start.column}, {r.end.line, r.end.column}};
      switch (I->getLevel()) {
      case DiagnosticsEngine::Ignored:
        // llvm_unreachable
        break;
      case DiagnosticsEngine::Note:
      case DiagnosticsEngine::Remark:
        ls_diag.severity = lsDiagnosticSeverity::Information;
        break;
      case DiagnosticsEngine::Warning:
        ls_diag.severity = lsDiagnosticSeverity::Warning;
        break;
      case DiagnosticsEngine::Error:
      case DiagnosticsEngine::Fatal:
        ls_diag.severity = lsDiagnosticSeverity::Error;
      }
      ls_diag.message = I->getMessage().str();
      for (const FixItHint &FixIt : I->getFixIts()) {
        lsTextEdit edit;
        edit.newText = FixIt.CodeToInsert;
        r = FromCharRange(SM, LangOpts, FixIt.RemoveRange.getAsRange());
        edit.range =
            lsRange{{r.start.line, r.start.column}, {r.end.line, r.end.column}};
        ls_diag.fixits_.push_back(edit);
      }
      ls_diags.push_back(ls_diag);
    }
    completion_manager->on_diagnostic_(path, ls_diags);
  }
}

}  // namespace

ClangCompleteManager::ClangCompleteManager(Project* project,
                                           WorkingFiles* working_files,
                                           OnDiagnostic on_diagnostic,
                                           OnDropped on_dropped)
    : project_(project),
      working_files_(working_files),
      on_diagnostic_(on_diagnostic),
      on_dropped_(on_dropped),
      preloaded_sessions_(kMaxPreloadedSessions),
      completion_sessions_(kMaxCompletionSessions) {
  std::thread([&]() {
    set_thread_name("comp-query");
    CompletionQueryMain(this);
  }).detach();
  std::thread([&]() {
    set_thread_name("comp-preload");
    CompletionPreloadMain(this);
  }).detach();
  std::thread([&]() {
    set_thread_name("diag-query");
    DiagnosticQueryMain(this);
  }).detach();
}

void ClangCompleteManager::CodeComplete(
    const lsRequestId& id,
    const lsTextDocumentPositionParams& completion_location,
    const OnComplete& on_complete) {
  completion_request_.PushBack(std::make_unique<CompletionRequest>(
      id, completion_location.textDocument, completion_location.position,
      on_complete));
}

void ClangCompleteManager::DiagnosticsUpdate(
    const lsTextDocumentIdentifier& document) {
  bool has = false;
  diagnostic_request_.Iterate([&](const DiagnosticRequest& request) {
    if (request.document.uri == document.uri)
      has = true;
  });
  if (!has)
    diagnostic_request_.PushBack(DiagnosticRequest{document},
                                 true /*priority*/);
}

void ClangCompleteManager::NotifyView(const std::string& filename) {
  //
  // On view, we reparse only if the file has not been parsed. The existence of
  // a CompletionSession instance implies the file is already parsed or will be
  // parsed soon.
  //

  // Only reparse the file if we create a new CompletionSession.
  if (EnsureCompletionOrCreatePreloadSession(filename))
    preload_requests_.PushBack(PreloadRequest(filename), true);
}

void ClangCompleteManager::NotifyEdit(const std::string& filename) {
  //
  // We treat an edit like a view, because the completion logic will handle
  // moving the CompletionSession instance from preloaded to completion
  // storage.
  //

  NotifyView(filename);
}

void ClangCompleteManager::NotifySave(const std::string& filename) {
  //
  // On save, always reparse.
  //

  EnsureCompletionOrCreatePreloadSession(filename);
  preload_requests_.PushBack(PreloadRequest(filename), true);
}

void ClangCompleteManager::NotifyClose(const std::string& filename) {
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
    const std::string& filename) {
  std::lock_guard<std::mutex> lock(sessions_lock_);

  // Check for an existing CompletionSession.
  if (preloaded_sessions_.TryGet(filename) ||
      completion_sessions_.TryGet(filename)) {
    return false;
  }

  // No CompletionSession, create new one.
  auto session = std::make_shared<CompletionSession>(
      project_->FindCompilationEntryForFile(filename), working_files_);
  preloaded_sessions_.Insert(session->file.filename, session);
  return true;
}

std::shared_ptr<CompletionSession> ClangCompleteManager::TryGetSession(
    const std::string& filename,
    bool mark_as_completion,
    bool create_if_needed) {
  std::lock_guard<std::mutex> lock(sessions_lock_);

  // Try to find a preloaded session.
  std::shared_ptr<CompletionSession> preloaded_session =
      preloaded_sessions_.TryGet(filename);

  if (preloaded_session) {
    // If this request is for a completion, we should move it to
    // |completion_sessions|.
    if (mark_as_completion) {
      preloaded_sessions_.TryTake(filename);
      completion_sessions_.Insert(filename, preloaded_session);
    }

    return preloaded_session;
  }

  // Try to find a completion session. If none create one.
  std::shared_ptr<CompletionSession> completion_session =
      completion_sessions_.TryGet(filename);
  if (!completion_session && create_if_needed) {
    completion_session = std::make_shared<CompletionSession>(
        project_->FindCompilationEntryForFile(filename), working_files_);
    completion_sessions_.Insert(filename, completion_session);
  }

  return completion_session;
}

void ClangCompleteManager::FlushSession(const std::string& filename) {
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
