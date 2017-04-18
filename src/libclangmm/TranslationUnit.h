#ifndef TRANSLATIONUNIT_H_
#define TRANSLATIONUNIT_H_
#include <clang-c/Index.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Index.h"
#include "Diagnostic.h"
#include "Tokens.h"
#include "CodeCompleteResults.h"
#include "Cursor.h"
#include "../language_server_api.h"

namespace clang {
  class TranslationUnit {
  public:
    TranslationUnit(IndexerConfig* config,
                    Index &index,
                    const std::string &filepath,
                    const std::vector<std::string>& arguments,
                    std::vector<CXUnsavedFile> unsaved_files,
                    unsigned flags);
    ~TranslationUnit();
    
    bool did_fail = false;

    void ReparseTranslationUnit(std::vector<CXUnsavedFile>& unsaved);

    clang::CodeCompleteResults get_code_completions(const std::string &buffer,
                                                    unsigned line_number, unsigned column);

    //std::vector<clang::Diagnostic> get_diagnostics();

    /*
    std::unique_ptr<Tokens> get_tokens(unsigned start_offset, unsigned end_offset);
    std::unique_ptr<Tokens> get_tokens(unsigned start_line, unsigned start_column,
                                       unsigned end_line, unsigned end_column);
    */
    Cursor document_cursor() const;

    /*
    clang::Cursor get_cursor(std::string path, unsigned offset);
    clang::Cursor get_cursor(std::string path, unsigned line, unsigned column);
    */

    CXTranslationUnit cx_tu;
  };
}  // namespace clang
#endif  // TRANSLATIONUNIT_H_

