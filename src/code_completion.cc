#include "code_completion.h"

#include "libclangmm/Utility.h"
#include "timer.h"

#include <algorithm>

namespace {
unsigned Flags() {
  /*
  return
    //CXTranslationUnit_Incomplete |
    CXTranslationUnit_PrecompiledPreamble |
    CXTranslationUnit_CacheCompletionResults |
    //CXTranslationUnit_ForSerialization |
    CXTranslationUnit_IncludeBriefCommentsInCodeCompletion |
    //CXTranslationUnit_CreatePreambleOnFirstParse |
    CXTranslationUnit_KeepGoing;
    */

  return
    CXTranslationUnit_CacheCompletionResults |
    CXTranslationUnit_PrecompiledPreamble |
    CXTranslationUnit_IncludeBriefCommentsInCodeCompletion |
    //CXTranslationUnit_CreatePreambleOnFirstParse |
    CXTranslationUnit_DetailedPreprocessingRecord;
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
    std::cerr << "Unhandled completion kind " << cursor_kind << std::endl;
    return lsCompletionItemKind::Text;
  }
}

std::string BuildLabelString(CXCompletionString completion_string) {
  std::string label;

  int num_chunks = clang_getNumCompletionChunks(completion_string);
  for (unsigned i = 0; i < num_chunks; ++i) {
    CXCompletionChunkKind kind = clang_getCompletionChunkKind(completion_string, i);
    if (kind == CXCompletionChunk_TypedText) {
      label += clang::ToString(clang_getCompletionChunkText(completion_string, i));
      break;
    }
  }

  return label;
}

std::string BuildDetailString(CXCompletionString completion_string) {
  std::string detail;

  int num_chunks = clang_getNumCompletionChunks(completion_string);
  for (unsigned i = 0; i < num_chunks; ++i) {
    CXCompletionChunkKind kind = clang_getCompletionChunkKind(completion_string, i);

    switch (kind) {
    case CXCompletionChunk_Optional: {
      CXCompletionString nested = clang_getCompletionChunkCompletionString(completion_string, i);
      detail += BuildDetailString(nested);
      break;
    }

    case CXCompletionChunk_Placeholder: {
      // TODO: send this info to vscode.
      CXString text = clang_getCompletionChunkText(completion_string, i);
      detail += clang::ToString(text);
      break;
    }

    case CXCompletionChunk_TypedText:
    case CXCompletionChunk_Text:
    case CXCompletionChunk_Informative:
    case CXCompletionChunk_CurrentParameter: {
      CXString text = clang_getCompletionChunkText(completion_string, i);
      detail += clang::ToString(text);
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
      break;
    case CXCompletionChunk_RightParen:
      detail += ")";
      break;
    case CXCompletionChunk_LeftBracket:
      detail += "]";
      break;
    case CXCompletionChunk_RightBracket:
      detail += "[";
      break;
    case CXCompletionChunk_LeftBrace:
      detail += "{";
      break;
    case CXCompletionChunk_RightBrace:
      detail += "}";
      break;
    case CXCompletionChunk_LeftAngle:
      detail += "<";
      break;
    case CXCompletionChunk_RightAngle:
      detail += ">";
      break;
    case CXCompletionChunk_Comma:
      detail += ", ";
      break;
    case CXCompletionChunk_Colon:
      detail += ":";
      break;
    case CXCompletionChunk_SemiColon:
      detail += ";";
      break;
    case CXCompletionChunk_Equal:
      detail += "=";
      break;
    case CXCompletionChunk_HorizontalSpace:
    case CXCompletionChunk_VerticalSpace:
      detail += " ";
      break;
    }
  }

  return detail;
}
}

CompletionSession::CompletionSession(const CompilationEntry& file, WorkingFiles* working_files) : file(file) {
  std::vector<CXUnsavedFile> unsaved = working_files->AsUnsavedFiles();

  std::vector<std::string> args = file.args;
  args.push_back("-x");
  args.push_back("c++");
  args.push_back("-fparse-all-comments");

  std::string sent_args = "";
  for (auto& arg : args)
    sent_args += arg + ", ";
  std::cerr << "Creating completion session with arguments " << sent_args << std::endl;

  // TODO: I think we crash when there are syntax errors.
  active_index = MakeUnique<clang::Index>(0 /*excludeDeclarationsFromPCH*/, 0 /*displayDiagnostics*/);
  active = MakeUnique<clang::TranslationUnit>(*active_index, file.filename, args, unsaved, Flags());
  std::cerr << "Done creating active; did_fail=" << active->did_fail << std::endl;
  //if (active->did_fail) {
  //  std::cerr << "Failed to create translation unit; trying again..." << std::endl;
  //  active = MakeUnique<clang::TranslationUnit>(*active_index, file.filename, args, unsaved, Flags());
  //}

  // Despite requesting clang create a precompiled header on the first parse in
  // Flags() via CXTranslationUnit_CreatePreambleOnFirstParse, it doesn't seem
  // to do so. Immediately reparsing will create one which reduces
  // clang_codeCompleteAt timing from 200ms to 20ms on simple files.
  // TODO: figure out why this is crashing so much
  if (!active->did_fail) {
    std::cerr << "Start reparse" << std::endl;
    active->ReparseTranslationUnit(unsaved);

    std::cerr << "Done reparse" << std::endl;
  }
}

CompletionSession::~CompletionSession() {}

void CompletionSession::Refresh(std::vector<CXUnsavedFile>& unsaved) {
  // TODO: Do this off the code completion thread so we don't block completions.
  active->ReparseTranslationUnit(unsaved);
}

CompletionManager::CompletionManager(Project* project, WorkingFiles* working_files) : project(project), working_files(working_files) {}

NonElidedVector<lsCompletionItem> CompletionManager::CodeComplete(const lsTextDocumentPositionParams& completion_location) {
  NonElidedVector<lsCompletionItem> ls_result;

  CompletionSession* session = GetOrOpenSession(completion_location.textDocument.uri.GetPath());

  unsigned line = completion_location.position.line + 1;
  unsigned column = completion_location.position.character + 1;

  std::cerr << std::endl;
  //std::cerr << "Completing at " << line << ":" << column << std::endl;

  std::vector<CXUnsavedFile> unsaved = working_files->AsUnsavedFiles();


  Timer timer;


  timer.Reset();
  CXCodeCompleteResults* cx_results = clang_codeCompleteAt(
    session->active->cx_tu,
    session->file.filename.c_str(), line, column,
    unsaved.data(), unsaved.size(),
    CXCodeComplete_IncludeMacros | CXCodeComplete_IncludeBriefComments);
  if (!cx_results) {
    std::cerr << "Code completion failed" << std::endl;
    return ls_result;
  }
  timer.ResetAndPrint("clangCodeCompleteAt");
  std::cerr << "Got " << cx_results->NumResults << " results" << std::endl;

  // TODO: for comments we could hack the unsaved buffer and transform // into ///

  ls_result.reserve(cx_results->NumResults);

  timer.Reset();
  for (int i = 0; i < cx_results->NumResults; ++i) {
    CXCompletionResult& result = cx_results->Results[i];

    //unsigned int is_incomplete;
    //CXCursorKind kind = clang_codeCompleteGetContainerKind(cx_results, &is_incomplete);
    //std::cerr << "clang_codeCompleteGetContainerKind kind=" << kind << " is_incomplete=" << is_incomplete << std::endl;

    // CXCursor_InvalidCode fo

    //clang_codeCompleteGetContexts
    //CXCompletionContext
    // clang_codeCompleteGetContexts
    //
    //CXCursorKind kind;
    //CXString str = clang_getCompletionParent(result.CompletionString, &kind);
    //std::cerr << "clang_getCompletionParent kind=" << kind << ", str=" << clang::ToString(str) << std::endl;
    // if global don't append now, append to end later

    // also clang_codeCompleteGetContainerKind

    // TODO: fill in more data
    lsCompletionItem ls_completion_item;

    // kind/label/detail/docs
    ls_completion_item.kind = GetCompletionKind(result.CursorKind);
    ls_completion_item.label = BuildLabelString(result.CompletionString);
    ls_completion_item.detail = BuildDetailString(result.CompletionString);
    ls_completion_item.documentation = clang::ToString(clang_getCompletionBriefComment(result.CompletionString));

    // Priority
    int priority = clang_getCompletionPriority(result.CompletionString);
    if (result.CursorKind == CXCursor_Destructor) {
      priority *= 100;
      //std::cerr << "Bumping[destructor] " << ls_completion_item.label << std::endl;
    }
    if (result.CursorKind == CXCursor_ConversionFunction ||
      (result.CursorKind == CXCursor_CXXMethod && StartsWith(ls_completion_item.label, "operator"))) {
      //std::cerr << "Bumping[conversion] " << ls_completion_item.label << std::endl;
      priority *= 100;
    }
    if (clang_getCompletionAvailability(result.CompletionString) != CXAvailability_Available) {
      //std::cerr << "Bumping[notavailable] " << ls_completion_item.label << std::endl;
      priority *= 100;
    }
    //std::cerr << "Adding kind=" << result.CursorKind << ", priority=" << ls_completion_item.priority_ << ", label=" << ls_completion_item.label << std::endl;

    // TODO: we can probably remove priority_ and our sort.
    ls_completion_item.sortText = uint64_t(priority);// std::to_string(ls_completion_item.priority_);

    ls_result.push_back(ls_completion_item);
  }
  timer.ResetAndPrint("Building completion results");

  clang_disposeCodeCompleteResults(cx_results);
  timer.ResetAndPrint("clang_disposeCodeCompleteResults ");

  return ls_result;

  // we should probably main two translation units, one for
  // serving current requests, and one that is reparsing (follow qtcreator)

  // todo: we need to run code completion on a separate thread from querydb
  // so thread layout looks like:
  //    - stdin                   # Reads data from stdin
  //    - stdout                  # Pushes data to stdout
  //    - querydb                 # Resolves index database queries.
  //    - complete_responder      # Handles low-latency code complete requests.
  //    - complete_parser         # Parses most recent document for future code complete requests.
  //    - indexer (many)          # Runs index jobs (for querydb updates)

  // use clang_codeCompleteAt
  //CXUnsavedFile
  // we need to setup CXUnsavedFile
  // The key to doing that is via
  //  - textDocument/didOpen
  //  - textDocument/didChange
  //  - textDocument/didClose

  // probably don't need
  //  - textDocument/willSave
}

CompletionSession* CompletionManager::GetOrOpenSession(const std::string& filename) {
  // Try to find existing session.
  for (auto& session : sessions) {
    if (session->file.filename == filename)
      return session.get();
  }

  // Create new session. Note that this will block.
  std::cerr << "Creating new code completion session for " << filename << std::endl;
  optional<CompilationEntry> entry = project->FindCompilationEntryForFile(filename);
  if (!entry) {
    std::cerr << "Unable to find compilation entry" << std::endl;
    entry = CompilationEntry();
    entry->filename = filename;
  }
  else {
    std::cerr << "Found compilation entry" << std::endl;
  }
  sessions.push_back(MakeUnique<CompletionSession>(*entry, working_files));
  return sessions[sessions.size() - 1].get();
}
