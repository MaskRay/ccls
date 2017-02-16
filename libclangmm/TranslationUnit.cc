#include "TranslationUnit.h"
#include "SourceLocation.h"
#include "Tokens.h"
#include "Utility.h"
#include <fstream>
#include <sstream>

#include <iostream> //TODO: remove
using namespace std; //TODO: remove

clang::TranslationUnit::TranslationUnit(Index &index, const std::string &file_path,
                                        const std::vector<std::string> &command_line_args,
                                        const std::string &buffer, unsigned flags) {
  std::vector<const char*> args;
  for(auto &a: command_line_args) {
    args.push_back(a.c_str());
  }
  
  CXUnsavedFile files[1];
  files[0].Filename=file_path.c_str();
  files[0].Contents=buffer.c_str();
  files[0].Length=buffer.size();
  
  cx_tu = clang_parseTranslationUnit(index.cx_index, file_path.c_str(), args.data(),
                                     args.size(), files, 1, flags);
}

clang::TranslationUnit::TranslationUnit(Index &index, const std::string &file_path,
                                        const std::vector<std::string> &command_line_args,
                                        unsigned flags) {
  std::vector<const char*> args;
  for(auto &a: command_line_args) {
    args.push_back(a.c_str());
  }
  
  cx_tu = clang_parseTranslationUnit(index.cx_index, file_path.c_str(), args.data(),
                                     args.size(), nullptr, 0, flags);
}

clang::TranslationUnit::~TranslationUnit() {
  clang_disposeTranslationUnit(cx_tu);
}

void clang::TranslationUnit::parse(Index &index, const std::string &file_path, 
                                   const std::vector<std::string> &command_line_args,
                                   const std::map<std::string, std::string>  &buffers, unsigned flags) {
  std::vector<CXUnsavedFile> files;
  for (auto &buffer : buffers) {
    CXUnsavedFile file;
    file.Filename = buffer.first.c_str();
    file.Contents = buffer.second.c_str();
    file.Length = buffer.second.size();
    files.push_back(file);
  }
  std::vector<const char*> args;
  for(auto &a: command_line_args) {
    args.push_back(a.c_str());
  }
  cx_tu = clang_parseTranslationUnit(index.cx_index, file_path.c_str(), args.data(), 
                                     args.size(), files.data(), files.size(), flags);
}

int clang::TranslationUnit::ReparseTranslationUnit(const std::string &buffer, unsigned flags) {
  CXUnsavedFile files[1];
  
  auto file_path=to_string(clang_getTranslationUnitSpelling(cx_tu));
  
  files[0].Filename=file_path.c_str();
  files[0].Contents=buffer.c_str();
  files[0].Length=buffer.size();
  
  return clang_reparseTranslationUnit(cx_tu, 1, files, flags);
}

unsigned clang::TranslationUnit::DefaultFlags() {
  auto flags=
    CXTranslationUnit_CacheCompletionResults |
    CXTranslationUnit_PrecompiledPreamble |
    CXTranslationUnit_Incomplete |
    CXTranslationUnit_IncludeBriefCommentsInCodeCompletion;
#if CINDEX_VERSION_MAJOR>0 || (CINDEX_VERSION_MAJOR==0 && CINDEX_VERSION_MINOR>=35)
  flags|=CXTranslationUnit_KeepGoing;
#endif
  return flags;
}

clang::CodeCompleteResults clang::TranslationUnit::get_code_completions(const std::string &buffer,
                                                                        unsigned line_number, unsigned column) {
  CodeCompleteResults results(cx_tu, buffer, line_number, column);
  return results;
}

std::vector<clang::Diagnostic> clang::TranslationUnit::get_diagnostics() {
  std::vector<Diagnostic> diagnostics;
  for(unsigned c=0;c<clang_getNumDiagnostics(cx_tu);c++) {
    CXDiagnostic clang_diagnostic=clang_getDiagnostic(cx_tu, c);
    diagnostics.emplace_back(Diagnostic(cx_tu, clang_diagnostic));
    clang_disposeDiagnostic(clang_diagnostic);
  }
  return diagnostics;
}

std::unique_ptr<clang::Tokens> clang::TranslationUnit::get_tokens(unsigned start_offset, unsigned end_offset) {
  auto path=clang::to_string(clang_getTranslationUnitSpelling(cx_tu));
  SourceLocation start_location(cx_tu, path, start_offset);
  SourceLocation end_location(cx_tu, path, end_offset);
  SourceRange range(start_location, end_location);
  return std::unique_ptr<Tokens>(new Tokens(cx_tu, range));
}

std::unique_ptr<clang::Tokens> clang::TranslationUnit::get_tokens(unsigned start_line, unsigned start_column,
                                                                  unsigned end_line, unsigned end_column) {
  auto path=to_string(clang_getTranslationUnitSpelling(cx_tu));
  SourceLocation start_location(cx_tu, path, start_line, start_column);
  SourceLocation end_location(cx_tu, path, end_line, end_column);
  SourceRange range(start_location, end_location);
  return std::unique_ptr<Tokens>(new Tokens(cx_tu, range));
}

clang::Cursor clang::TranslationUnit::get_cursor(std::string path, unsigned offset) {
  SourceLocation location(cx_tu, path, offset);
  return Cursor(clang_getCursor(cx_tu, location.cx_location));
}

clang::Cursor clang::TranslationUnit::get_cursor(std::string path, unsigned line, unsigned column) {
  SourceLocation location(cx_tu, path, line, column);
  return Cursor(clang_getCursor(cx_tu, location.cx_location));
}
