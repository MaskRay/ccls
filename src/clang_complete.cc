#include "clang_complete.h"

#include "clang_utils.h"
#include "platform.h"
#include "timer.h"

#include <loguru.hpp>

#include <algorithm>
#include <thread>

namespace {

unsigned Flags() {
  // TODO: use clang_defaultEditingTranslationUnitOptions()?
  return CXTranslationUnit_Incomplete | CXTranslationUnit_KeepGoing |
         CXTranslationUnit_CacheCompletionResults |
         CXTranslationUnit_PrecompiledPreamble |
         CXTranslationUnit_IncludeBriefCommentsInCodeCompletion |
         CXTranslationUnit_DetailedPreprocessingRecord
#if !defined(_WIN32)
         // For whatever reason, CreatePreambleOnFirstParse causes clang to
         // become very crashy on windows.
         // TODO: do more investigation, submit fixes to clang.
         | CXTranslationUnit_CreatePreambleOnFirstParse
#endif
      ;
}

unsigned GetCompletionPriority(const CXCompletionString& str,
                               CXCursorKind result_kind,
                               const std::string& label) {
  unsigned priority = clang_getCompletionPriority(str);

  // XXX: What happens if priority overflows?
  if (result_kind == CXCursor_Destructor) {
    priority *= 100;
  }
  if (result_kind == CXCursor_ConversionFunction ||
      (result_kind == CXCursor_CXXMethod && StartsWith(label, "operator"))) {
    priority *= 100;
  }
  if (clang_getCompletionAvailability(str) != CXAvailability_Available) {
    priority *= 100;
  }
  return priority;
}

/*
bool IsCallKind(CXCursorKind kind) {
  switch (kind) {
    case CXCursor_ObjCInstanceMethodDecl:
    case CXCursor_CXXMethod:
    case CXCursor_FunctionTemplate:
    case CXCursor_FunctionDecl:
    case CXCursor_Constructor:
    case CXCursor_Destructor:
    case CXCursor_ConversionFunction:
      return true;
    default:
      return false;
  }
}
*/

lsCompletionItemKind GetCompletionKind(CXCursorKind cursor_kind) {
  switch (cursor_kind) {
    case CXCursor_UnexposedDecl:
      return lsCompletionItemKind::Text;

    case CXCursor_ObjCInstanceMethodDecl:
    case CXCursor_CXXMethod:
    case CXCursor_ObjCClassMethodDecl:
      return lsCompletionItemKind::Method;

    case CXCursor_FunctionTemplate:
    case CXCursor_FunctionDecl:
      return lsCompletionItemKind::Function;

    case CXCursor_Constructor:
    case CXCursor_Destructor:
    case CXCursor_ConversionFunction:
      return lsCompletionItemKind::Constructor;

    case CXCursor_FieldDecl:
      return lsCompletionItemKind::Field;

    case CXCursor_VarDecl:
    case CXCursor_ParmDecl:
    case CXCursor_ObjCIvarDecl:
      return lsCompletionItemKind::Variable;

    case CXCursor_UnionDecl:
    case CXCursor_ClassTemplate:
    case CXCursor_ClassTemplatePartialSpecialization:
    case CXCursor_ClassDecl:
    case CXCursor_UsingDeclaration:
    case CXCursor_TypedefDecl:
    case CXCursor_TypeAliasDecl:
    case CXCursor_TypeAliasTemplateDecl:
    case CXCursor_ObjCCategoryDecl:
    case CXCursor_ObjCProtocolDecl:
    case CXCursor_ObjCPropertyDecl:
    case CXCursor_ObjCImplementationDecl:
    case CXCursor_ObjCCategoryImplDecl:
      return lsCompletionItemKind::Class;

    case CXCursor_EnumDecl:
      return lsCompletionItemKind::Enum;

    case CXCursor_MacroInstantiation:
    case CXCursor_MacroDefinition:
    case CXCursor_ObjCInterfaceDecl:
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

      // return lsCompletionItemKind::Property;
      // return lsCompletionItemKind::Unit;
      // return lsCompletionItemKind::Value;
      // return lsCompletionItemKind::Keyword;
      // return lsCompletionItemKind::Snippet;
      // return lsCompletionItemKind::Color;
      // return lsCompletionItemKind::File;

    case CXCursor_NotImplemented:
    case CXCursor_OverloadCandidate:
      return lsCompletionItemKind::Text;

    case CXCursor_EnumConstantDecl:
      return lsCompletionItemKind::EnumMember;

    case CXCursor_StructDecl:
      return lsCompletionItemKind::Struct;

    default:
      LOG_S(WARNING) << "Unhandled completion kind " << cursor_kind;
      return lsCompletionItemKind::Text;
  }
}

// |do_insert|: if |!do_insert|, do not append strings to |insert| after
// a placeholder.
void BuildDetailString(CXCompletionString completion_string,
                       std::string& label,
                       std::string& detail,
                       std::string& insert,
                       bool& do_insert,
                       lsInsertTextFormat& format,
                       std::vector<std::string>* parameters,
                       bool include_snippets) {
  int num_chunks = clang_getNumCompletionChunks(completion_string);
  for (int i = 0; i < num_chunks; ++i) {
    CXCompletionChunkKind kind =
        clang_getCompletionChunkKind(completion_string, i);

    switch (kind) {
      case CXCompletionChunk_Optional: {
        CXCompletionString nested =
            clang_getCompletionChunkCompletionString(completion_string, i);
        BuildDetailString(nested, label, detail, insert, do_insert, format,
                          parameters, include_snippets);
        break;
      }

      case CXCompletionChunk_Placeholder: {
        std::string text =
            ToString(clang_getCompletionChunkText(completion_string, i));
        parameters->push_back(text);
        detail += text;
        // Add parameter declarations as snippets if enabled
        if (include_snippets) {
          insert +=
              "${" + std::to_string(parameters->size()) + ":" + text + "}";
          format = lsInsertTextFormat::Snippet;
        } else
          do_insert = false;
        break;
      }

      case CXCompletionChunk_CurrentParameter:
        // We have our own parsing logic for active parameter. This doesn't seem
        // to be very reliable.
        break;

      case CXCompletionChunk_TypedText: {
        std::string text =
            ToString(clang_getCompletionChunkText(completion_string, i));
        label = text;
        detail += text;
        if (do_insert)
          insert += text;
        break;
      }

      case CXCompletionChunk_Text: {
        std::string text =
            ToString(clang_getCompletionChunkText(completion_string, i));
        detail += text;
        if (do_insert)
          insert += text;
        break;
      }

      case CXCompletionChunk_Informative: {
        detail += ToString(clang_getCompletionChunkText(completion_string, i));
        break;
      }

      case CXCompletionChunk_ResultType: {
        CXString text = clang_getCompletionChunkText(completion_string, i);
        std::string new_detail = ToString(text) + detail + " ";
        detail = new_detail;
        break;
      }

      case CXCompletionChunk_LeftParen:
        detail += "(";
        if (do_insert)
          insert += "(";
        break;
      case CXCompletionChunk_RightParen:
        detail += ")";
        if (do_insert)
          insert += ")";
        break;
      case CXCompletionChunk_LeftBracket:
        detail += "[";
        if (do_insert)
          insert += "[";
        break;
      case CXCompletionChunk_RightBracket:
        detail += "]";
        if (do_insert)
          insert += "]";
        break;
      case CXCompletionChunk_LeftBrace:
        detail += "{";
        if (do_insert)
          insert += "{";
        break;
      case CXCompletionChunk_RightBrace:
        detail += "}";
        if (do_insert)
          insert += "}";
        break;
      case CXCompletionChunk_LeftAngle:
        detail += "<";
        if (do_insert)
          insert += "<";
        break;
      case CXCompletionChunk_RightAngle:
        detail += ">";
        if (do_insert)
          insert += ">";
        break;
      case CXCompletionChunk_Comma:
        detail += ", ";
        if (do_insert)
          insert += ", ";
        break;
      case CXCompletionChunk_Colon:
        detail += ":";
        if (do_insert)
          insert += ":";
        break;
      case CXCompletionChunk_SemiColon:
        detail += ";";
        if (do_insert)
          insert += ";";
        break;
      case CXCompletionChunk_Equal:
        detail += "=";
        if (do_insert)
          insert += "=";
        break;
      case CXCompletionChunk_HorizontalSpace:
      case CXCompletionChunk_VerticalSpace:
        detail += " ";
        if (do_insert)
          insert += " ";
        break;
    }
  }
}

void TryEnsureDocumentParsed(ClangCompleteManager* manager,
                             std::shared_ptr<CompletionSession> session,
                             std::unique_ptr<ClangTranslationUnit>* tu,
                             ClangIndex* index) {
  // Nothing to do. We already have a translation unit.
  if (*tu)
    return;

  std::vector<std::string> args = session->file.args;

  // -fspell-checking enables FixIts for, ie, misspelled types.
  if (!AnyStartsWith(args, "-fno-spell-checking") &&
      !AnyStartsWith(args, "-fspell-checking")) {
    args.push_back("-fspell-checking");
  }

  WorkingFiles::Snapshot snapshot =
      session->working_files->AsSnapshot({StripFileType(session->file.filename)});
  std::vector<CXUnsavedFile> unsaved = snapshot.AsUnsavedFiles();

  LOG_S(INFO) << "Creating completion session with arguments "
              << StringJoin(args);
  *tu = ClangTranslationUnit::Create(index, session->file.filename, args,
                                     unsaved, Flags());

  // Build diagnostics.
  if (manager->config_->diagnosticsOnParse && *tu) {
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
      optional<lsDiagnostic> diagnostic = BuildAndDisposeDiagnostic(
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

void CompletionParseMain(ClangCompleteManager* completion_manager) {
  while (true) {
    // Fetching the completion request blocks until we have a request.
    ClangCompleteManager::ParseRequest request =
        completion_manager->parse_requests_.Dequeue();

    // If we don't get a session then that means we don't care about the file
    // anymore - abandon the request.
    std::shared_ptr<CompletionSession> session =
        completion_manager->TryGetSession(request.path,
                                          false /*mark_as_completion*/,
                                          false /*create_if_needed*/);
    if (!session)
      continue;

    // If we've parsed it more recently than the request time, don't bother
    // reparsing.
    if (session->tu_last_parsed_at &&
        *session->tu_last_parsed_at > request.request_time) {
      continue;
    }

    std::unique_ptr<ClangTranslationUnit> parsing;
    TryEnsureDocumentParsed(completion_manager, session, &parsing,
                            &session->index);

    // Activate new translation unit.
    // tu_last_parsed_at is only read by this thread, so it doesn't need to be
    // under the mutex.
    session->tu_last_parsed_at = std::chrono::high_resolution_clock::now();
    std::lock_guard<std::mutex> lock(session->tu_lock);
    session->tu = std::move(parsing);
  }
}

void CompletionQueryMain(ClangCompleteManager* completion_manager) {
  while (true) {
    // Fetching the completion request blocks until we have a request.
    std::unique_ptr<ClangCompleteManager::CompletionRequest> request =
        completion_manager->completion_request_.Take();
    std::string path = request->document.uri.GetPath();

    std::shared_ptr<CompletionSession> session =
        completion_manager->TryGetSession(path, true /*mark_as_completion*/,
                                          true /*create_if_needed*/);

    std::lock_guard<std::mutex> lock(session->tu_lock);
    Timer timer;
    TryEnsureDocumentParsed(completion_manager, session, &session->tu,
                            &session->index);
    timer.ResetAndPrint("[complete] TryEnsureDocumentParsed");

    // It is possible we failed to create the document despite
    // |TryEnsureDocumentParsed|.
    if (!session->tu)
      continue;

    timer.Reset();
    WorkingFiles::Snapshot snapshot =
        completion_manager->working_files_->AsSnapshot({StripFileType(path)});
    std::vector<CXUnsavedFile> unsaved = snapshot.AsUnsavedFiles();
    timer.ResetAndPrint("[complete] Creating WorkingFile snapshot");


    // Emit code completion data.
    if (request->position) {
      // Language server is 0-based, clang is 1-based.
      unsigned line = request->position->line + 1;
      unsigned column = request->position->character + 1;

      timer.Reset();
      unsigned const kCompleteOptions =
          CXCodeComplete_IncludeMacros | CXCodeComplete_IncludeBriefComments;
      CXCodeCompleteResults* cx_results = clang_codeCompleteAt(
          session->tu->cx_tu, session->file.filename.c_str(), line, column,
          unsaved.data(), (unsigned)unsaved.size(), kCompleteOptions);
      timer.ResetAndPrint("[complete] clangCodeCompleteAt");
      if (!cx_results) {
        if (request->on_complete)
          request->on_complete({}, false /*is_cached_result*/);
        continue;
      }

      {
        if (request->on_complete) {
          std::vector<lsCompletionItem> ls_result;
          ls_result.reserve(cx_results->NumResults);

          timer.Reset();
          for (unsigned i = 0; i < cx_results->NumResults; ++i) {
            CXCompletionResult& result = cx_results->Results[i];

            // TODO: Try to figure out how we can hide base method calls without
            // also hiding method implementation assistance, ie,
            //
            //    void Foo::* {
            //    }
            //

            if (clang_getCompletionAvailability(result.CompletionString) ==
                CXAvailability_NotAvailable)
              continue;

            // TODO: fill in more data
            lsCompletionItem ls_completion_item;

            bool do_insert = true;
            // kind/label/detail/docs/sortText
            ls_completion_item.kind = GetCompletionKind(result.CursorKind);
            BuildDetailString(
                result.CompletionString, ls_completion_item.label,
                ls_completion_item.detail, ls_completion_item.insertText,
                do_insert, ls_completion_item.insertTextFormat,
                &ls_completion_item.parameters_,
                completion_manager->config_->client.snippetSupport);
            if (completion_manager->config_->client.snippetSupport &&
                ls_completion_item.insertTextFormat ==
                    lsInsertTextFormat::Snippet) {
              ls_completion_item.insertText += "$0";
            }

            ls_completion_item.documentation = ToString(
                clang_getCompletionBriefComment(result.CompletionString));

            ls_completion_item.priority_ = GetCompletionPriority(
                result.CompletionString, result.CursorKind,
                ls_completion_item.label);

            ls_result.push_back(ls_completion_item);
          }

          timer.ResetAndPrint("[complete] Building " +
                              std::to_string(ls_result.size()) +
                              " completion results");

          request->on_complete(ls_result, false /*is_cached_result*/);
        }
      }

      // Make sure |ls_results| is destroyed before clearing |cx_results|.
      clang_disposeCodeCompleteResults(cx_results);
    }

    // Emit diagnostics.
    if (request->emit_diagnostics) {
      // TODO: before emitting diagnostics check if we have another completion
      // request and think about servicing that first, because it may be much
      // faster than reparsing the document.
      // TODO: have a separate thread for diagnostics?

      timer.Reset();
      session->tu =
          ClangTranslationUnit::Reparse(std::move(session->tu), unsaved);
      timer.ResetAndPrint("[complete] clang_reparseTranslationUnit");
      if (!session->tu) {
        LOG_S(ERROR) << "Reparsing translation unit for diagnostics failed for "
                     << path;
        continue;
      }

      size_t num_diagnostics = clang_getNumDiagnostics(session->tu->cx_tu);
      std::vector<lsDiagnostic> ls_diagnostics;
      ls_diagnostics.reserve(num_diagnostics);
      for (unsigned i = 0; i < num_diagnostics; ++i) {
        CXDiagnostic cx_diag = clang_getDiagnostic(session->tu->cx_tu, i);
        optional<lsDiagnostic> diagnostic =
            BuildAndDisposeDiagnostic(cx_diag, path);
        // Filter messages like "too many errors emitted, stopping now
        // [-ferror-limit=]" which has line = 0 and got subtracted by 1 after
        // conversion to lsDiagnostic
        if (diagnostic && diagnostic->range.start.line >= 0)
          ls_diagnostics.push_back(*diagnostic);
      }
      completion_manager->on_diagnostic_(session->file.filename,
                                         ls_diagnostics);

      /*
      timer.Reset();
      completion_manager->on_index_(session->tu.get(), unsaved,
                                    session->file.filename, session->file.args);
      timer.ResetAndPrint("[complete] Reindex file");
      */
    }

    continue;
  }
}

}  // namespace

CompletionSession::CompletionSession(const Project::Entry& file,
                                     WorkingFiles* working_files)
    : file(file),
      working_files(working_files),
      index(0 /*excludeDeclarationsFromPCH*/, 0 /*displayDiagnostics*/) {}

CompletionSession::~CompletionSession() {}

ClangCompleteManager::ParseRequest::ParseRequest(const std::string& path)
    : request_time(std::chrono::high_resolution_clock::now()), path(path) {}

ClangCompleteManager::ClangCompleteManager(Config* config,
                                           Project* project,
                                           WorkingFiles* working_files,
                                           OnDiagnostic on_diagnostic,
                                           OnIndex on_index)
    : config_(config),
      project_(project),
      working_files_(working_files),
      on_diagnostic_(on_diagnostic),
      on_index_(on_index),
      preloaded_sessions_(kMaxPreloadedSessions),
      completion_sessions_(kMaxCompletionSessions) {
  new std::thread([&]() {
    SetCurrentThreadName("completequery");
    CompletionQueryMain(this);
  });

  new std::thread([&]() {
    SetCurrentThreadName("completeparse");
    CompletionParseMain(this);
  });
}

ClangCompleteManager::~ClangCompleteManager() {}

void ClangCompleteManager::CodeComplete(
    const lsTextDocumentPositionParams& completion_location,
    const OnComplete& on_complete) {
  completion_request_.WithLock(
      [&](std::unique_ptr<CompletionRequest>& request_storage) {
        // Ensure that we have a request.
        if (!request_storage)
          request_storage = MakeUnique<CompletionRequest>();

        // Make the request send out code completion information.
        request_storage->document = completion_location.textDocument;
        request_storage->position = completion_location.position;
        request_storage->on_complete = on_complete;
      });
}

void ClangCompleteManager::DiagnosticsUpdate(
    const lsTextDocumentIdentifier& document) {
  completion_request_.WithLock(
      [&](std::unique_ptr<CompletionRequest>& request_storage) {
        // Ensure that we have a request.
        if (!request_storage)
          request_storage = MakeUnique<CompletionRequest>();

        // Make the request emit diagnostics.
        request_storage->document = document;
        request_storage->emit_diagnostics = true;
      });
}

void ClangCompleteManager::NotifyView(const std::string& filename) {
  //
  // On view, we reparse only if the file has not been parsed. The existence of
  // a CompletionSession instance implies the file is already parsed or will be
  // parsed soon.
  //

  // Only reparse the file if we create a new CompletionSession.
  if (EnsureCompletionOrCreatePreloadSession(filename))
    parse_requests_.PriorityEnqueue(ParseRequest(filename));
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
  parse_requests_.PriorityEnqueue(ParseRequest(filename));
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
      assert(!completion_sessions_.TryGet(filename));
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
