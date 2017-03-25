#ifndef CODECOMPLETERESULTS_H_
#define CODECOMPLETERESULTS_H_
#include <clang-c/Index.h>
#include <map>
#include <string>
#include "CompletionString.h"

namespace clang {
  class CodeCompleteResults {
    friend class TranslationUnit;
    
    CodeCompleteResults(CXTranslationUnit &cx_tu, const std::string &buffer,
                        unsigned line_num, unsigned column);
  public:
    ~CodeCompleteResults();
    CompletionString get(unsigned index) const;
    unsigned size() const;
    std::string get_usr() const;

    CXCodeCompleteResults *cx_results;
  };
}  // namespace clang
#endif  // CODECOMPLETERESULTS_H_
