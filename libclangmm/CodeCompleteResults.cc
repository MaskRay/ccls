#include "CodeCompleteResults.h"
#include "CompletionString.h"
#include <exception>
#include "Utility.h"

clang::CodeCompleteResults::CodeCompleteResults(CXTranslationUnit &cx_tu, 
                                                const std::string &buffer,
                                                unsigned line_num, unsigned column) {
  CXUnsavedFile files[1];
  auto file_path=to_string(clang_getTranslationUnitSpelling(cx_tu));
  files[0].Filename = file_path.c_str();
  files[0].Contents = buffer.c_str();
  files[0].Length = buffer.size();

  cx_results = clang_codeCompleteAt(cx_tu,
                                  file_path.c_str(),
                                  line_num,
                                  column,
                                  files,
                                  1,
                                  clang_defaultCodeCompleteOptions()|CXCodeComplete_IncludeBriefComments);
  if(cx_results!=nullptr)
    clang_sortCodeCompletionResults(cx_results->Results, cx_results->NumResults);
}

clang::CodeCompleteResults::~CodeCompleteResults() {
  clang_disposeCodeCompleteResults(cx_results);
}

unsigned clang::CodeCompleteResults::size() const {
  if(cx_results==nullptr)
    return 0;
  return cx_results->NumResults;
}

clang::CompletionString clang::CodeCompleteResults::get(unsigned i) const {
  if (i >= size()) {
    throw std::invalid_argument("clang::CodeCompleteResults::get(unsigned i): i>=size()");
  }
  return CompletionString(cx_results->Results[i].CompletionString);
}

std::string clang::CodeCompleteResults::get_usr() const {
  return to_string(clang_codeCompleteGetContainerUSR(cx_results));
}
