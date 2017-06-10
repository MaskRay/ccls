#include "clang_complete.h"

#include "clang_utils.h"
#include "libclangmm/Utility.h"
#include "platform.h"
#include "timer.h"

#include <algorithm>
#include <thread>

namespace {
unsigned Flags() {
  // TODO: use clang_defaultEditingTranslationUnitOptions()?
  return
    CXTranslationUnit_Incomplete |
    CXTranslationUnit_KeepGoing |
    CXTranslationUnit_CacheCompletionResults |
    CXTranslationUnit_PrecompiledPreamble |
    CXTranslationUnit_IncludeBriefCommentsInCodeCompletion
#if !defined(_WIN32)
    // For whatever reason, CreatePreambleOnFirstParse causes clang to become
    // very crashy on windows.
    // TODO: do more investigation, submit fixes to clang.
    | CXTranslationUnit_CreatePreambleOnFirstParse
#endif
    ;
}

int GetCompletionPriority(const CXCompletionString& str, CXCursorKind result_kind, const std::string& label) {
  int priority = clang_getCompletionPriority(str);
  if (result_kind == CXCursor_Destructor) {
    priority *= 100;
    //std::cerr << "Bumping[destructor] " << ls_completion_item.label << std::endl;
  }
  if (result_kind == CXCursor_ConversionFunction ||
    (result_kind  == CXCursor_CXXMethod && StartsWith(label, "operator"))) {
    //std::cerr << "Bumping[conversion] " << ls_completion_item.label << std::endl;
    priority *= 100;
  }
  if (clang_getCompletionAvailability(str) != CXAvailability_Available) {
    //std::cerr << "Bumping[notavailable] " << ls_completion_item.label << std::endl;
    priority *= 100;
  }
  return priority;
}

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

lsCompletionItemKind GetCompletionKind(CXCursorKind cursor_kind) {
  switch (cursor_kind) {

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

      //return lsCompletionItemKind::Property;
      //return lsCompletionItemKind::Unit;
      //return lsCompletionItemKind::Value;
      //return lsCompletionItemKind::Keyword;
      //return lsCompletionItemKind::Snippet;
      //return lsCompletionItemKind::Color;
      //return lsCompletionItemKind::File;

    case CXCursor_NotImplemented:
      return lsCompletionItemKind::Text;

    default:
      std::cerr << "[complete] Unhandled completion kind " << cursor_kind << std::endl;
      return lsCompletionItemKind::Text;
  }
}

void BuildDetailString(CXCompletionString completion_string, std::string& label, std::string& detail, std::string& insert, std::vector<std::string>* parameters) {
  int num_chunks = clang_getNumCompletionChunks(completion_string);
  for (int i = 0; i < num_chunks; ++i) {
    CXCompletionChunkKind kind = clang_getCompletionChunkKind(completion_string, i);

    switch (kind) {
    case CXCompletionChunk_Optional: {
      CXCompletionString nested = clang_getCompletionChunkCompletionString(completion_string, i);
      BuildDetailString(nested, label, detail, insert, parameters);
      break;
    }

    case CXCompletionChunk_Placeholder: {
      // TODO: insert $0 at end of snippet
      std::string text = clang::ToString(clang_getCompletionChunkText(completion_string, i));
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
      std::string text = clang::ToString(clang_getCompletionChunkText(completion_string, i));
      label = text;
      detail += text;
      insert += text;
      break;
    }

    case CXCompletionChunk_Text: {
      std::string text = clang::ToString(clang_getCompletionChunkText(completion_string, i));
      detail += text;
      insert += text;
      break;
    }

    case CXCompletionChunk_Informative: {
      detail += clang::ToString(clang_getCompletionChunkText(completion_string, i));
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

void EnsureDocumentParsed(CompletionSession* session,
                          std::unique_ptr<clang::TranslationUnit>* tu,
                          clang::Index* index) {
  // Nothing to do. We already have a translation unit.
  if (*tu)
    return;

  std::vector<std::string> args = session->file.args;
  args.push_back("-fparse-all-comments");

  std::vector<CXUnsavedFile> unsaved = session->working_files->AsUnsavedFiles();

  std::cerr << "[complete] Creating completion session with arguments " << StringJoin(args) << std::endl;
  *tu = MakeUnique<clang::TranslationUnit>(index, session->file.filename, args, unsaved, Flags());
  std::cerr << "[complete] Done creating active; did_fail=" << (*tu)->did_fail << std::endl;
}

void CompletionParseMain(ClangCompleteManager* completion_manager) {
  while (true) {
    // Fetching the completion request blocks until we have a request.
    ClangCompleteManager::ParseRequest request = completion_manager->parse_requests_.Dequeue();

    // If we don't get a session then that means we don't care about the file
    // anymore - abandon the request.
    CompletionSession* session = completion_manager->TryGetSession(request.path, false /*create_if_needed*/);
    if (!session)
      continue;

    // If we've parsed it more recently than the request time, don't bother
    // reparsing.
    if (session->tu_last_parsed_at &&
        *session->tu_last_parsed_at > request.request_time) {
      continue;
    }

    std::unique_ptr<clang::TranslationUnit> parsing;
    EnsureDocumentParsed(session, &parsing, &session->index);

    // Activate new translation unit.
    // tu_last_parsed_at is only read by this thread, so it doesn't need to be under the mutex.
    session->tu_last_parsed_at = std::chrono::high_resolution_clock::now();
    std::lock_guard<std::mutex> lock(session->tu_lock);
    session->tu = std::move(parsing);
  }
}

void CompletionQueryMain(ClangCompleteManager* completion_manager) {
  while (true) {
    // Fetching the completion request blocks until we have a request.
    std::unique_ptr<ClangCompleteManager::CompletionRequest> request = completion_manager->completion_request_.Take();
    std::string path = request->location.textDocument.uri.GetPath();

    CompletionSession* session = completion_manager->TryGetSession(path, true /*create_if_needed*/);

    std::lock_guard<std::mutex> lock(session->tu_lock);
    EnsureDocumentParsed(session, &session->tu, &session->index);

    // Language server is 0-based, clang is 1-based.
    unsigned line = request->location.position.line + 1;
    unsigned column = request->location.position.character + 1;

    std::cerr << "[complete] Completing at " << line << ":" << column << std::endl;

    Timer timer;

    std::vector<CXUnsavedFile> unsaved = completion_manager->working_files_->AsUnsavedFiles();
    timer.ResetAndPrint("[complete] Fetching unsaved files");

    timer.Reset();
    unsigned const kCompleteOptions = CXCodeComplete_IncludeMacros | CXCodeComplete_IncludeBriefComments;
    CXCodeCompleteResults* cx_results = clang_codeCompleteAt(
      session->tu->cx_tu,
      session->file.filename.c_str(), line, column,
      unsaved.data(), (unsigned)unsaved.size(),
      kCompleteOptions);
    if (!cx_results) {
      timer.ResetAndPrint("[complete] Code completion failed");
      request->on_complete({}, {});
      continue;
    }

    timer.ResetAndPrint("[complete] clangCodeCompleteAt");
    std::cerr << "[complete] Got " << cx_results->NumResults << " results" << std::endl;

    NonElidedVector<lsCompletionItem> ls_result;
    ls_result.reserve(cx_results->NumResults);

    timer.Reset();
    for (unsigned i = 0; i < cx_results->NumResults; ++i) {
      CXCompletionResult& result = cx_results->Results[i];

      // TODO: Try to figure out how we can hide base method calls without also
      // hiding method implementation assistance, ie,
      //
      //    void Foo::* {
      //    }
      //

      if (clang_getCompletionAvailability(result.CompletionString) == CXAvailability_NotAvailable)
        continue;

      // TODO: fill in more data
      lsCompletionItem ls_completion_item;

      // kind/label/detail/docs/sortText
      ls_completion_item.kind = GetCompletionKind(result.CursorKind);
      BuildDetailString(result.CompletionString, ls_completion_item.label, ls_completion_item.detail, ls_completion_item.insertText, &ls_completion_item.parameters_);
      ls_completion_item.documentation = clang::ToString(clang_getCompletionBriefComment(result.CompletionString));
      ls_completion_item.sortText = (const char)uint64_t(GetCompletionPriority(result.CompletionString, result.CursorKind, ls_completion_item.label));

      // If this function is slow we can skip building insertText at the cost of some code duplication.
      if (!IsCallKind(result.CursorKind))
        ls_completion_item.insertText = "";

      ls_result.push_back(ls_completion_item);
    }
    timer.ResetAndPrint("[complete] Building " + std::to_string(ls_result.size()) + " completion results");

    // Build diagnostics.
    NonElidedVector<lsDiagnostic> ls_diagnostics;
    timer.Reset();
    /*
    unsigned num_diagnostics = clang_codeCompleteGetNumDiagnostics(cx_results);
    std::cerr << "!! There are " + std::to_string(num_diagnostics) + " diagnostics to build\n";
    for (unsigned i = 0; i < num_diagnostics; ++i) {
      std::cerr << "!! Building diagnostic " + std::to_string(i) + "\n";
      CXDiagnostic cx_diag = clang_codeCompleteGetDiagnostic(cx_results, i);
      optional<lsDiagnostic> diagnostic = BuildDiagnostic(cx_diag);
      if (diagnostic)
        ls_diagnostics.push_back(*diagnostic);
    }
    timer.ResetAndPrint("[complete] Build diagnostics");
    */


    clang_disposeCodeCompleteResults(cx_results);
    timer.ResetAndPrint("[complete] clang_disposeCodeCompleteResults");

    request->on_complete(ls_result, ls_diagnostics);

    continue;
  }
}

}  // namespace

CompletionSession::CompletionSession(const Project::Entry& file, WorkingFiles* working_files)
  : file(file), working_files(working_files), index(0 /*excludeDeclarationsFromPCH*/, 0 /*displayDiagnostics*/) {}

CompletionSession::~CompletionSession() {}

LruSessionCache::LruSessionCache(int max_entries) : max_entries_(max_entries) {}

CompletionSession* LruSessionCache::TryGetEntry(const std::string& filename) {
  for (int i = 0; i < entries_.size(); ++i) {
    if (entries_[i]->file.filename == filename)
      return entries_[i].get();
  }
  return nullptr;
}

std::unique_ptr<CompletionSession> LruSessionCache::TryTakeEntry(const std::string& filename) {
  for (int i = 0; i < entries_.size(); ++i) {
    if (entries_[i]->file.filename == filename) {
      std::unique_ptr<CompletionSession> result = std::move(entries_[i]);
      entries_.erase(entries_.begin() + i);
      return result;
    }
  }
  return nullptr;
}

void LruSessionCache::InsertEntry(std::unique_ptr<CompletionSession> session) {
  if (entries_.size() >= max_entries_)
    entries_.pop_back();
  entries_.insert(entries_.begin(), std::move(session));
}

ClangCompleteManager::ParseRequest::ParseRequest(const std::string& path)
  : request_time(std::chrono::high_resolution_clock::now()), path(path) {}

ClangCompleteManager::ClangCompleteManager(Config* config, Project* project, WorkingFiles* working_files)
    : config_(config), project_(project), working_files_(working_files),
      view_sessions_(kMaxViewSessions), edit_sessions_(kMaxEditSessions) {
  new std::thread([&]() {
    SetCurrentThreadName("completequery");
    CompletionQueryMain(this);
  });

  new std::thread([&]() {
    SetCurrentThreadName("completeparse");
    CompletionParseMain(this);
  });
}

void ClangCompleteManager::CodeComplete(const lsTextDocumentPositionParams& completion_location, const OnComplete& on_complete) {
  // completion thread will create the CompletionSession if needed.

  auto request = MakeUnique<CompletionRequest>();
  request->location = completion_location;
  request->on_complete = on_complete;
  completion_request_.Set(std::move(request));
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

  std::cerr << "[complete] Creating new edit code completion session for " << filename << std::endl;
  view_sessions_.InsertEntry(MakeUnique<CompletionSession>(
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

  if (std::unique_ptr<CompletionSession> session = view_sessions_.TryTakeEntry(filename)) {
    edit_sessions_.InsertEntry(std::move(session));
  }

  std::cerr << "[complete] Creating new edit code completion session for " << filename << std::endl;
  edit_sessions_.InsertEntry(MakeUnique<CompletionSession>(
    project_->FindCompilationEntryForFile(filename), working_files_));
  parse_requests_.PriorityEnqueue(ParseRequest(filename));
}

void ClangCompleteManager::NotifySave(const std::string& filename) {
  //
  // On save, always reparse.
  //

  std::lock_guard<std::mutex> lock(sessions_lock_);

  if (!edit_sessions_.TryGetEntry(filename)) {
    std::cerr << "[complete] Creating new edit code completion session for " << filename << std::endl;
    edit_sessions_.InsertEntry(MakeUnique<CompletionSession>(
      project_->FindCompilationEntryForFile(filename), working_files_));
  }

  parse_requests_.PriorityEnqueue(ParseRequest(filename));
}

CompletionSession* ClangCompleteManager::TryGetSession(const std::string& filename, bool create_if_needed) {
  std::lock_guard<std::mutex> lock(sessions_lock_);

  CompletionSession* session = edit_sessions_.TryGetEntry(filename);

  if (!session)
    session = view_sessions_.TryGetEntry(filename);

  if (!session && create_if_needed) {
    // Create new session. Default to edited_sessions_ since invoking code
    // completion almost certainly implies an edit.
    edit_sessions_.InsertEntry(MakeUnique<CompletionSession>(
      project_->FindCompilationEntryForFile(filename), working_files_));
    session = edit_sessions_.TryGetEntry(filename);
  }

  return session;
}
