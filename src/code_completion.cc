#include "code_completion.h"

#include "clang_utils.h"
#include "libclangmm/Utility.h"
#include "platform.h"
#include "timer.h"

#include <algorithm>
#include <thread>

namespace {
unsigned Flags() {
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
                          std::unique_ptr<clang::Index>* index) {
  // Nothing to do. We already have a translation unit and an index.
  if (*tu && *index)
    return;

  std::vector<std::string> args = session->file.args;
  args.push_back("-fparse-all-comments");

  std::vector<CXUnsavedFile> unsaved = session->working_files->AsUnsavedFiles();

  std::cerr << "[complete] Creating completion session with arguments " << StringJoin(args) << std::endl;
  *index = MakeUnique<clang::Index>(0 /*excludeDeclarationsFromPCH*/, 0 /*displayDiagnostics*/);
  *tu = MakeUnique<clang::TranslationUnit>(*index->get(), session->file.filename, args, unsaved, Flags());
  std::cerr << "[complete] Done creating active; did_fail=" << (*tu)->did_fail << std::endl;
}

void CompletionParseMain(CompletionManager* completion_manager) {
  while (true) {
    // Fetching the completion request blocks until we have a request.
    std::unique_ptr<std::string> path = completion_manager->reparse_request.Take();
    
    CompletionSession* session = completion_manager->GetOrOpenSession(*path);
    std::unique_ptr<clang::TranslationUnit> parsing;
    std::unique_ptr<clang::Index> parsing_index;

    EnsureDocumentParsed(session, &parsing, &parsing_index);

    // Swap out active.
    std::lock_guard<std::mutex> lock(session->usage_lock);
    session->active = std::move(parsing);
    session->active_index = std::move(parsing_index);
  }
}

void CompletionQueryMain(CompletionManager* completion_manager) {
  while (true) {
    // Fetching the completion request blocks until we have a request.
    std::unique_ptr<CompletionManager::CompletionRequest> request = completion_manager->completion_request.Take();


    CompletionSession* session = completion_manager->GetOrOpenSession(request->location.textDocument.uri.GetPath());
    std::lock_guard<std::mutex> lock(session->usage_lock);

    EnsureDocumentParsed(session, &session->active, &session->active_index);

    unsigned line = request->location.position.line + 1;
    unsigned column = request->location.position.character + 1;

    std::cerr << std::endl;
    std::cerr << "[complete] Completing at " << line << ":" << column << std::endl;

    Timer timer;

    std::vector<CXUnsavedFile> unsaved = completion_manager->working_files->AsUnsavedFiles();
    timer.ResetAndPrint("[complete] Fetching unsaved files");

    timer.Reset();
    CXCodeCompleteResults* cx_results = clang_codeCompleteAt(
      session->active->cx_tu,
      session->file.filename.c_str(), line, column,
      unsaved.data(), unsaved.size(),
      CXCodeComplete_IncludeMacros | CXCodeComplete_IncludeBriefComments);
    if (!cx_results) {
      std::cerr << "[complete] Code completion failed" << std::endl;
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
      ls_completion_item.sortText = uint64_t(GetCompletionPriority(result.CompletionString, result.CursorKind, ls_completion_item.label));
      
      // If this function is slow we can skip building insertText at the cost of some code duplication.
      if (!IsCallKind(result.CursorKind))
        ls_completion_item.insertText = "";

      ls_result.push_back(ls_completion_item);
    }
    timer.ResetAndPrint("[complete] Building " + std::to_string(ls_result.size()) + " completion results");

    // Build diagnostics.
    NonElidedVector<lsDiagnostic> ls_diagnostics;
    timer.Reset();
    unsigned num_diagnostics = clang_codeCompleteGetNumDiagnostics(cx_results);
    for (unsigned i = 0; i < num_diagnostics; ++i) {
      optional<lsDiagnostic> diagnostic = BuildDiagnostic(clang_codeCompleteGetDiagnostic(cx_results, i));
      if (diagnostic)
        ls_diagnostics.push_back(*diagnostic);
    }
    timer.ResetAndPrint("[complete] Build diagnostics");

    clang_disposeCodeCompleteResults(cx_results);
    timer.ResetAndPrint("[complete] clang_disposeCodeCompleteResults");

    request->on_complete(ls_result, ls_diagnostics);

    continue;
  }
}

}  // namespace

CompletionSession::CompletionSession(const Project::Entry& file, WorkingFiles* working_files)
  : file(file), working_files(working_files) {}

CompletionSession::~CompletionSession() {}

CompletionManager::CompletionManager(Config* config, Project* project, WorkingFiles* working_files)
    : config(config), project(project), working_files(working_files) {
  new std::thread([&]() {
    SetCurrentThreadName("completequery");
    CompletionQueryMain(this);
  });

  new std::thread([&]() {
    SetCurrentThreadName("completeparse");
    CompletionParseMain(this);
  });
}

void CompletionManager::CodeComplete(const lsTextDocumentPositionParams& completion_location, const OnComplete& on_complete) {
  auto request = MakeUnique<CompletionRequest>();
  request->location = completion_location;
  request->on_complete = on_complete;
  completion_request.Set(std::move(request));
}

CompletionSession* CompletionManager::GetOrOpenSession(const std::string& filename) {
  // Try to find existing session.
  for (auto& session : sessions) {
    if (session->file.filename == filename)
      return session.get();
  }

  // Create new session. Note that this will block.
  std::cerr << "[complete] Creating new code completion session for " << filename << std::endl;
  optional<Project::Entry> entry = project->FindCompilationEntryForFile(filename);
  if (!entry) {
    std::cerr << "[complete] Unable to find compilation entry" << std::endl;
    entry = Project::Entry();
    entry->filename = filename;
  }
  else {
    std::cerr << "[complete] Found compilation entry" << std::endl;
  }
  sessions.push_back(MakeUnique<CompletionSession>(*entry, working_files));
  return sessions[sessions.size() - 1].get();
}

void CompletionManager::UpdateActiveSession(const std::string& filename) {
  // Drop all sessions except for |filename|.
  for (auto& session : sessions) {
    if (session->file.filename == filename)
      continue;

    std::lock_guard<std::mutex> lock(session->usage_lock);
    session->active.reset();
    session->active_index.reset();
  }

  // Reparse |filename|.
  // TODO: Instead of actually reparsing it, see if we can hook into the
  // indexer and steal the translation unit from there..
  reparse_request.Set(MakeUnique<std::string>(filename));
}