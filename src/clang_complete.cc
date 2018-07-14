#include "clang_complete.h"

#include "clang_utils.h"
#include "filesystem.hh"
#include "log.hh"
#include "platform.h"

#include <clang/Sema/CodeCompleteConsumer.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/Threading.h>
using namespace clang;
using namespace llvm;

#include <algorithm>
#include <thread>

namespace {

unsigned Flags() {
  // TODO: use clang_defaultEditingTranslationUnitOptions()?
  return CXTranslationUnit_Incomplete | CXTranslationUnit_KeepGoing |
         CXTranslationUnit_CacheCompletionResults |
         CXTranslationUnit_PrecompiledPreamble |
         CXTranslationUnit_IncludeBriefCommentsInCodeCompletion |
         CXTranslationUnit_DetailedPreprocessingRecord |
         CXTranslationUnit_CreatePreambleOnFirstParse;
}

std::string StripFileType(const std::string& path) {
  SmallString<128> Ret;
  sys::path::append(Ret, sys::path::parent_path(path), sys::path::stem(path));
  return Ret.str();
}

unsigned GetCompletionPriority(const CodeCompletionString& CCS,
                               CXCursorKind result_kind,
                               const std::optional<std::string>& typedText) {
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

    for (auto i = out_first; i < out.size(); ++i)
      out[i].label += text;

    if (Kind == CodeCompletionString::CK_Informative)
      continue;

    for (auto i = out_first; i < out.size(); ++i) {
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

  if (result_type.empty())
    return;

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

void TryEnsureDocumentParsed(ClangCompleteManager* manager,
                             std::shared_ptr<CompletionSession> session,
                             std::unique_ptr<ClangTranslationUnit>* tu,
                             ClangIndex* index,
                             bool emit_diag) {
  // Nothing to do. We already have a translation unit.
  if (*tu)
    return;

  std::vector<std::string> args = session->file.args;

  // -fspell-checking enables FixIts for, ie, misspelled types.
  if (!std::count(args.begin(), args.end(), "-fno-spell-checking") &&
      !std::count(args.begin(), args.end(), "-fspell-checking")) {
    args.push_back("-fspell-checking");
  }

  WorkingFiles::Snapshot snapshot = session->working_files->AsSnapshot(
      {StripFileType(session->file.filename)});
  std::vector<CXUnsavedFile> unsaved = snapshot.AsUnsavedFiles();

  LOG_S(INFO) << "Creating completion session with arguments "
              << StringJoin(args, " ");
  *tu = ClangTranslationUnit::Create(index, session->file.filename, args,
                                     unsaved, Flags());

  // Build diagnostics.
  if (g_config->diagnostics.onParse && *tu) {
    // If we're emitting diagnostics, do an immediate reparse, otherwise we will
    // emit stale/bad diagnostics.
    *tu = ClangTranslationUnit::Reparse(std::move(*tu), unsaved);
    if (!*tu) {
      LOG_S(ERROR) << "Reparsing translation unit for diagnostics failed for "
                   << session->file.filename;
      return;
    }

    std::vector<lsDiagnostic> ls_diagnostics;
    unsigned num_diagnostics = clang_getNumDiagnostics((*tu)->cx_tu);
    for (unsigned i = 0; i < num_diagnostics; ++i) {
      std::optional<lsDiagnostic> diagnostic = BuildAndDisposeDiagnostic(
          clang_getDiagnostic((*tu)->cx_tu, i), session->file.filename);
      // Filter messages like "too many errors emitted, stopping now
      // [-ferror-limit=]" which has line = 0 and got subtracted by 1 after
      // conversion to lsDiagnostic
      if (diagnostic && diagnostic->range.start.line >= 0)
        ls_diagnostics.push_back(*diagnostic);
    }
    manager->on_diagnostic_(session->file.filename, ls_diagnostics);
  }
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
    TryEnsureDocumentParsed(completion_manager, session, &parsing, &tu->index,
                            true);

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
    TryEnsureDocumentParsed(completion_manager, session, &session->completion.tu,
                            &session->completion.index, false);

    // It is possible we failed to create the document despite
    // |TryEnsureDocumentParsed|.
    if (!session->completion.tu)
      continue;

    WorkingFiles::Snapshot snapshot =
        completion_manager->working_files_->AsSnapshot({StripFileType(path)});
    std::vector<CXUnsavedFile> unsaved = snapshot.AsUnsavedFiles();

    unsigned const kCompleteOptions =
        CXCodeComplete_IncludeMacros | CXCodeComplete_IncludeBriefComments;
    CXCodeCompleteResults* cx_results = clang_codeCompleteAt(
        session->completion.tu->cx_tu, session->file.filename.c_str(),
        request->position.line + 1, request->position.character + 1,
        unsaved.data(), (unsigned)unsaved.size(), kCompleteOptions);
    if (!cx_results) {
      request->on_complete({}, false /*is_cached_result*/);
      continue;
    }

    std::vector<lsCompletionItem> ls_result;
    // this is a guess but can be larger in case of std::optional
    // parameters, as they may be expanded into multiple items
    ls_result.reserve(cx_results->NumResults);

    for (unsigned i = 0; i < cx_results->NumResults; ++i) {
      CXCompletionResult& result = cx_results->Results[i];
      auto CCS = (CodeCompletionString *)result.CompletionString;
      if (CCS->getAvailability() == CXAvailability_NotAvailable)
        continue;

      lsCompletionItem ls_item;
      ls_item.kind = GetCompletionKind(result.CursorKind);
      if (const char* brief = CCS->getBriefComment())
        ls_item.documentation = brief;

      // label/detail/filterText/insertText/priority
      if (g_config->completion.detailedLabel) {
        ls_item.detail = CCS->getParentContextName().str();

        auto first_idx = ls_result.size();
        ls_result.push_back(ls_item);

        // label/filterText/insertText
        BuildCompletionItemTexts(ls_result, *CCS,
                                 g_config->client.snippetSupport);

        for (auto i = first_idx; i < ls_result.size(); ++i) {
          if (g_config->client.snippetSupport &&
              ls_result[i].insertTextFormat == lsInsertTextFormat::Snippet) {
            ls_result[i].insertText += "$0";
          }

          ls_result[i].priority_ = GetCompletionPriority(
              *CCS, result.CursorKind, ls_result[i].filterText);
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
            GetCompletionPriority(*CCS, result.CursorKind, ls_item.label);
        ls_result.push_back(ls_item);
      }
    }

    request->on_complete(ls_result, false /*is_cached_result*/);

    // Make sure |ls_results| is destroyed before clearing |cx_results|.
    clang_disposeCodeCompleteResults(cx_results);
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
    TryEnsureDocumentParsed(
        completion_manager, session, &session->diagnostics.tu,
        &session->diagnostics.index, false /*emit_diagnostics*/);

    // It is possible we failed to create the document despite
    // |TryEnsureDocumentParsed|.
    if (!session->diagnostics.tu)
      continue;

    WorkingFiles::Snapshot snapshot =
        completion_manager->working_files_->AsSnapshot({StripFileType(path)});
    std::vector<CXUnsavedFile> unsaved = snapshot.AsUnsavedFiles();

    // Emit diagnostics.
    session->diagnostics.tu = ClangTranslationUnit::Reparse(
        std::move(session->diagnostics.tu), unsaved);
    if (!session->diagnostics.tu) {
      LOG_S(ERROR) << "Reparsing translation unit for diagnostics failed for "
                   << path;
      continue;
    }

    size_t num_diagnostics =
        clang_getNumDiagnostics(session->diagnostics.tu->cx_tu);
    std::vector<lsDiagnostic> ls_diagnostics;
    ls_diagnostics.reserve(num_diagnostics);
    for (unsigned i = 0; i < num_diagnostics; ++i) {
      CXDiagnostic cx_diag =
          clang_getDiagnostic(session->diagnostics.tu->cx_tu, i);
      std::optional<lsDiagnostic> diagnostic =
          BuildAndDisposeDiagnostic(cx_diag, path);
      // Filter messages like "too many errors emitted, stopping now
      // [-ferror-limit=]" which has line = 0 and got subtracted by 1 after
      // conversion to lsDiagnostic
      if (diagnostic && diagnostic->range.start.line >= 0)
        ls_diagnostics.push_back(*diagnostic);
    }
    completion_manager->on_diagnostic_(path, ls_diagnostics);
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
