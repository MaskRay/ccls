#include "TranslationUnit.h"
#include "Tokens.h"
#include "Utility.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>

namespace clang {

TranslationUnit::TranslationUnit(
  Index &index,
  const std::string& filepath,
  const std::vector<std::string>& arguments,
  std::vector<CXUnsavedFile> unsaved_files,
  unsigned flags) {

  std::vector<const char*> args;
  for (const std::string& a : arguments) {
    args.push_back(a.c_str());
  }

  CXErrorCode error_code = clang_parseTranslationUnit2(
    index.cx_index,
    filepath.c_str(),
    args.data(), args.size(),
    unsaved_files.data(), unsaved_files.size(),
    flags, &cx_tu);
  
  switch (error_code) {
  case CXError_Success:
    did_fail = false;
    break;
  case CXError_Failure:
    std::cerr << "libclang generic failure for " << filepath << std::endl;
    did_fail = true;
    break;
  case CXError_Crashed:
    std::cerr << "libclang crashed for " << filepath << std::endl;
    did_fail = true;
    break;
  case CXError_InvalidArguments:
    std::cerr << "libclang had invalid arguments for " << filepath << std::endl;
    did_fail = true;
    break;
  case CXError_ASTReadError:
    std::cerr << "libclang had ast read error for " << filepath << std::endl;
    did_fail = true;
    break;
  }
}

TranslationUnit::~TranslationUnit() {
  clang_disposeTranslationUnit(cx_tu);
}

void TranslationUnit::ReparseTranslationUnit(std::vector<CXUnsavedFile>& unsaved) {
  int error_code = clang_reparseTranslationUnit(cx_tu, unsaved.size(), unsaved.data(), clang_defaultReparseOptions(cx_tu));
  switch (error_code) {
  case CXError_Success:
    did_fail = false;
    break;
  case CXError_Failure:
    std::cerr << "libclang reparse generic failure" << std::endl;
    did_fail = true;
    break;
  case CXError_Crashed:
    std::cerr << "libclang reparse crashed " << std::endl;
    did_fail = true;
    break;
  case CXError_InvalidArguments:
    std::cerr << "libclang reparse had invalid arguments" << std::endl;
    did_fail = true;
    break;
  case CXError_ASTReadError:
    std::cerr << "libclang reparse had ast read error" << std::endl;
    did_fail = true;
    break;
  }
}

CodeCompleteResults TranslationUnit::get_code_completions(
  const std::string& buffer, unsigned line_number, unsigned column) {
  CodeCompleteResults results(cx_tu, buffer, line_number, column);
  return results;
}

/*
std::vector<Diagnostic> TranslationUnit::get_diagnostics() {
  std::vector<Diagnostic> diagnostics;
  for (unsigned c = 0; c < clang_getNumDiagnostics(cx_tu); c++) {
    CXDiagnostic clang_diagnostic = clang_getDiagnostic(cx_tu, c);
    diagnostics.emplace_back(Diagnostic(cx_tu, clang_diagnostic));
    clang_disposeDiagnostic(clang_diagnostic);
  }
  return diagnostics;
}
*/

/*
std::unique_ptr<Tokens> TranslationUnit::get_tokens(unsigned start_offset, unsigned end_offset) {
  auto path = ToString(clang_getTranslationUnitSpelling(cx_tu));
  SourceLocation start_location(cx_tu, path, start_offset);
  SourceLocation end_location(cx_tu, path, end_offset);
  SourceRange range(start_location, end_location);
  return std::unique_ptr<Tokens>(new Tokens(cx_tu, range));
}

std::unique_ptr<Tokens> TranslationUnit::get_tokens(unsigned start_line, unsigned start_column, unsigned end_line, unsigned end_column) {
  auto path = ToString(clang_getTranslationUnitSpelling(cx_tu));
  SourceLocation start_location(cx_tu, path, start_line, start_column);
  SourceLocation end_location(cx_tu, path, end_line, end_column);
  SourceRange range(start_location, end_location);
  return std::unique_ptr<Tokens>(new Tokens(cx_tu, range));
}
*/

Cursor TranslationUnit::document_cursor() const {
  return Cursor(clang_getTranslationUnitCursor(cx_tu));
}

/*
Cursor TranslationUnit::get_cursor(std::string path, unsigned offset) {
  SourceLocation location(cx_tu, path, offset);
  return Cursor(clang_getCursor(cx_tu, location.cx_location));
}

Cursor TranslationUnit::get_cursor(std::string path, unsigned line, unsigned column) {
  SourceLocation location(cx_tu, path, line, column);
  return Cursor(clang_getCursor(cx_tu, location.cx_location));
}
*/

}