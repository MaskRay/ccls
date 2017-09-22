#include "clang_complete.h"

#include "clang_utils.h"
#include "libclangmm/Utility.h"
#include "platform.h"
#include "timer.h"

#include <loguru.hpp>

#include <algorithm>
#include <thread>

/*
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
*/

namespace {

#if false
constexpr int kBacktraceBufferSize = 300;

void EmitBacktrace() {
   void* buffer[kBacktraceBufferSize];
   int nptrs = backtrace(buffer, kBacktraceBufferSize);

   fprintf(stderr, "backtrace() returned %d addresses\n", nptrs);

   /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
      would produce similar output to the following: */

   char** strings = backtrace_symbols(buffer, nptrs);
   if (!strings) {
       perror("Failed to emit backtrace");
       exit(EXIT_FAILURE);
   }

   for (int j = 0; j < nptrs; j++)
       fprintf(stderr, "%s\n", strings[j]);

   free(strings);
}
#endif

unsigned Flags() {
  // TODO: use clang_defaultEditingTranslationUnitOptions()?
  return CXTranslationUnit_Incomplete | CXTranslationUnit_KeepGoing |
         CXTranslationUnit_CacheCompletionResults |
         CXTranslationUnit_PrecompiledPreamble |
         CXTranslationUnit_IncludeBriefCommentsInCodeCompletion
#if !defined(_WIN32)
         // For whatever reason, CreatePreambleOnFirstParse causes clang to
         // become very crashy on windows.
         // TODO: do more investigation, submit fixes to clang.
         | CXTranslationUnit_CreatePreambleOnFirstParse
#endif
      ;
}

int GetCompletionPriority(const CXCompletionString& str,
                          CXCursorKind result_kind,
                          const std::string& label) {
  int priority = clang_getCompletionPriority(str);
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
      return lsCompletionItemKind::Variable;

    case CXCursor_UnionDecl:
    case CXCursor_ClassTemplate:
    case CXCursor_ClassTemplatePartialSpecialization:
    case CXCursor_ClassDecl:
    case CXCursor_StructDecl:
    case CXCursor_UsingDeclaration:
    case CXCursor_TypedefDecl:
    case CXCursor_TypeAliasDecl:
    case CXCursor_TypeAliasTemplateDecl:
      return lsCompletionItemKind::Class;

    case CXCursor_EnumConstantDecl:
    case CXCursor_EnumDecl:
      return lsCompletionItemKind::Enum;

    case CXCursor_MacroInstantiation:
    case CXCursor_MacroDefinition:
      return lsCompletionItemKind::Interface;

    case CXCursor_Namespace:
    case CXCursor_NamespaceAlias:
    case CXCursor_NamespaceRef:
      return lsCompletionItemKind::Module;

    case CXCursor_MemberRef:
    case CXCursor_TypeRef:
      return lsCompletionItemKind::Reference;

      // return lsCompletionItemKind::Property;
      // return lsCompletionItemKind::Unit;
      // return lsCompletionItemKind::Value;
      // return lsCompletionItemKind::Keyword;
      // return lsCompletionItemKind::Snippet;
      // return lsCompletionItemKind::Color;
      // return lsCompletionItemKind::File;

    case CXCursor_NotImplemented:
      return lsCompletionItemKind::Text;

    default:
      LOG_S(WARNING) << "Unhandled completion kind " << cursor_kind;
      return lsCompletionItemKind::Text;
  }
}

void BuildDetailString(CXCompletionString completion_string,
                       std::string& label,
                       std::string& detail,
                       std::string& insert,
                       std::vector<std::string>* parameters) {
  int num_chunks = clang_getNumCompletionChunks(completion_string);
  for (int i = 0; i < num_chunks; ++i) {
    CXCompletionChunkKind kind =
        clang_getCompletionChunkKind(completion_string, i);

    switch (kind) {
      case CXCompletionChunk_Optional: {
        CXCompletionString nested =
            clang_getCompletionChunkCompletionString(completion_string, i);
        BuildDetailString(nested, label, detail, insert, parameters);
        break;
      }

      case CXCompletionChunk_Placeholder: {
        std::string text =
            clang::ToString(clang_getCompletionChunkText(completion_string, i));
        parameters->push_back(text);
        detail += text;
        insert += "${" + std::to_string(parameters->size()) + ":" + text + "}";
        break;
      }

      case CXCompletionChunk_CurrentParameter:
        // We have our own parsing logic for active parameter. This doesn't seem
        // to be very reliable.
        break;

      case CXCompletionChunk_TypedText: {
        std::string text =
            clang::ToString(clang_getCompletionChunkText(completion_string, i));
        label = text;
        detail += text;
        insert += text;
        break;
      }

      case CXCompletionChunk_Text: {
        std::string text =
            clang::ToString(clang_getCompletionChunkText(completion_string, i));
        detail += text;
        insert += text;
        break;
      }

      case CXCompletionChunk_Informative: {
        detail +=
            clang::ToString(clang_getCompletionChunkText(completion_string, i));
        break;
      }

      case CXCompletionChunk_ResultType: {
        CXString text = clang_getCompletionChunkText(completion_string, i);
        std::string new_detail = clang::ToString(text) + detail + " ";
        detail = new_detail;
        break;
      }

      case CXCompletionChunk_LeftParen:
        detail += "(";
        insert += "(";
        break;
      case CXCompletionChunk_RightParen:
        detail += ")";
        insert += ")";
        break;
      case CXCompletionChunk_LeftBracket:
        detail += "[";
        insert += "[";
        break;
      case CXCompletionChunk_RightBracket:
        detail += "]";
        insert += "]";
        break;
      case CXCompletionChunk_LeftBrace:
        detail += "{";
        insert += "{";
        break;
      case CXCompletionChunk_RightBrace:
        detail += "}";
        insert += "}";
        break;
      case CXCompletionChunk_LeftAngle:
        detail += "<";
        insert += "<";
        break;
      case CXCompletionChunk_RightAngle:
        detail += ">";
        insert += ">";
        break;
      case CXCompletionChunk_Comma:
        detail += ", ";
        insert += ", ";
        break;
      case CXCompletionChunk_Colon:
        detail += ":";
        insert += ":";
        break;
      case CXCompletionChunk_SemiColon:
        detail += ";";
        insert += ";";
        break;
      case CXCompletionChunk_Equal:
        detail += "=";
        insert += "=";
        break;
      case CXCompletionChunk_HorizontalSpace:
      case CXCompletionChunk_VerticalSpace:
        detail += " ";
        insert += " ";
        break;
    }
  }
}

void EnsureDocumentParsed(ClangCompleteManager* manager,
                          std::shared_ptr<CompletionSession> session,
                          std::unique_ptr<clang::TranslationUnit>* tu,
                          clang::Index* index) {
  // Nothing to do. We already have a translation unit.
  if (*tu)
    return;

  std::vector<std::string> args = session->file.args;
  args.push_back("-fparse-all-comments");

  std::vector<CXUnsavedFile> unsaved = session->working_files->AsUnsavedFiles();

  LOG_S(INFO) << "Creating completion session with arguments "
              << StringJoin(args);
  *tu = MakeUnique<clang::TranslationUnit>(index, session->file.filename, args,
                                           unsaved, Flags());

  // Build diagnostics.
  if (manager->config_->diagnosticsOnParse && !(*tu)->did_fail) {
    NonElidedVector<lsDiagnostic> ls_diagnostics;
    unsigned num_diagnostics = clang_getNumDiagnostics((*tu)->cx_tu);
    for (unsigned i = 0; i < num_diagnostics; ++i) {
      optional<lsDiagnostic> diagnostic = BuildAndDisposeDiagnostic(
          clang_getDiagnostic((*tu)->cx_tu, i), session->file.filename);
      if (diagnostic)
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
                                          false /*create_if_needed*/);
    if (!session)
      continue;

    // If we've parsed it more recently than the request time, don't bother
    // reparsing.
    if (session->tu_last_parsed_at &&
        *session->tu_last_parsed_at > request.request_time) {
      continue;
    }

    std::unique_ptr<clang::TranslationUnit> parsing;
    EnsureDocumentParsed(completion_manager, session, &parsing,
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
        completion_manager->TryGetSession(path, true /*create_if_needed*/);

    std::lock_guard<std::mutex> lock(session->tu_lock);
    EnsureDocumentParsed(completion_manager, session, &session->tu,
                         &session->index);
    Timer timer;

    std::vector<CXUnsavedFile> unsaved =
        completion_manager->working_files_->AsUnsavedFiles();

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
          NonElidedVector<lsCompletionItem> ls_result;
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

            // kind/label/detail/docs/sortText
            ls_completion_item.kind = GetCompletionKind(result.CursorKind);
            BuildDetailString(result.CompletionString, ls_completion_item.label,
                              ls_completion_item.detail,
                              ls_completion_item.insertText,
                              &ls_completion_item.parameters_);
            ls_completion_item.insertText += "$0";
            ls_completion_item.documentation = clang::ToString(
                clang_getCompletionBriefComment(result.CompletionString));
            ls_completion_item.sortText =
                (const char)uint64_t(GetCompletionPriority(
                    result.CompletionString, result.CursorKind,
                    ls_completion_item.label));

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
      timer.Reset();
      clang_reparseTranslationUnit(
          session->tu->cx_tu, unsaved.size(), unsaved.data(),
          clang_defaultReparseOptions(session->tu->cx_tu));
      timer.ResetAndPrint("[complete] clang_reparseTranslationUnit");

      size_t num_diagnostics = clang_getNumDiagnostics(session->tu->cx_tu);
      NonElidedVector<lsDiagnostic> ls_diagnostics;
      ls_diagnostics.reserve(num_diagnostics);
      for (unsigned i = 0; i < num_diagnostics; ++i) {
        CXDiagnostic cx_diag = clang_getDiagnostic(session->tu->cx_tu, i);
        optional<lsDiagnostic> diagnostic =
            BuildAndDisposeDiagnostic(cx_diag, path);
        if (diagnostic)
          ls_diagnostics.push_back(*diagnostic);
      }
      completion_manager->on_diagnostic_(session->file.filename,
                                         ls_diagnostics);
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

LruSessionCache::LruSessionCache(int max_entries) : max_entries_(max_entries) {}

std::shared_ptr<CompletionSession> LruSessionCache::TryGetEntry(
    const std::string& filename) {
  for (int i = 0; i < entries_.size(); ++i) {
    if (entries_[i]->file.filename == filename)
      return entries_[i];
  }
  return nullptr;
}

std::shared_ptr<CompletionSession> LruSessionCache::TryTakeEntry(
    const std::string& filename) {
  for (int i = 0; i < entries_.size(); ++i) {
    if (entries_[i]->file.filename == filename) {
      std::shared_ptr<CompletionSession> result = entries_[i];
      entries_.erase(entries_.begin() + i);
      return result;
    }
  }
  return nullptr;
}

void LruSessionCache::InsertEntry(std::shared_ptr<CompletionSession> session) {
  if (entries_.size() && entries_.size() >= max_entries_)
    entries_.pop_back();
  entries_.insert(entries_.begin(), session);
}

ClangCompleteManager::ParseRequest::ParseRequest(const std::string& path)
    : request_time(std::chrono::high_resolution_clock::now()), path(path) {}

ClangCompleteManager::ClangCompleteManager(Config* config,
                                           Project* project,
                                           WorkingFiles* working_files,
                                           OnDiagnostic on_diagnostic)
    : config_(config),
      project_(project),
      working_files_(working_files),
      on_diagnostic_(on_diagnostic),
      view_sessions_(kMaxViewSessions),
      edit_sessions_(kMaxEditSessions) {
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

  std::lock_guard<std::mutex> lock(sessions_lock_);

  if (view_sessions_.TryGetEntry(filename))
    return;

  view_sessions_.InsertEntry(std::make_shared<CompletionSession>(
      project_->FindCompilationEntryForFile(filename), working_files_));
  parse_requests_.Enqueue(ParseRequest(filename));
}

void ClangCompleteManager::NotifyEdit(const std::string& filename) {
  //
  // On edit, we reparse only if the file has not been parsed. The existence of
  // a CompletionSession instance implies the file is already parsed or will be
  // parsed soon.
  //

  std::lock_guard<std::mutex> lock(sessions_lock_);

  if (edit_sessions_.TryGetEntry(filename))
    return;

  std::shared_ptr<CompletionSession> session =
      view_sessions_.TryTakeEntry(filename);
  if (session) {
    edit_sessions_.InsertEntry(session);
  } else {
    edit_sessions_.InsertEntry(std::make_shared<CompletionSession>(
        project_->FindCompilationEntryForFile(filename), working_files_));
    parse_requests_.PriorityEnqueue(ParseRequest(filename));
  }
}

void ClangCompleteManager::NotifySave(const std::string& filename) {
  //
  // On save, always reparse.
  //

  std::lock_guard<std::mutex> lock(sessions_lock_);

  if (!edit_sessions_.TryGetEntry(filename)) {
    edit_sessions_.InsertEntry(std::make_shared<CompletionSession>(
        project_->FindCompilationEntryForFile(filename), working_files_));
  }

  parse_requests_.PriorityEnqueue(ParseRequest(filename));
}

std::shared_ptr<CompletionSession> ClangCompleteManager::TryGetSession(
    const std::string& filename,
    bool create_if_needed) {
  std::lock_guard<std::mutex> lock(sessions_lock_);

  std::shared_ptr<CompletionSession> session =
      edit_sessions_.TryGetEntry(filename);

  if (!session)
    session = view_sessions_.TryGetEntry(filename);

  if (!session && create_if_needed) {
    // Create new session. Default to edited_sessions_ since invoking code
    // completion almost certainly implies an edit.
    edit_sessions_.InsertEntry(std::make_shared<CompletionSession>(
        project_->FindCompilationEntryForFile(filename), working_files_));
    session = edit_sessions_.TryGetEntry(filename);
  }

  return session;
}
