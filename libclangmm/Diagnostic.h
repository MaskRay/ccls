#ifndef DIAGNOSTIC_H_
#define DIAGNOSTIC_H_
#include <string>
#include <vector>
#include <clang-c/Index.h>
#include "SourceRange.h"

namespace clang {
  class Diagnostic {
    friend class TranslationUnit;
    Diagnostic(CXTranslationUnit& cx_tu, CXDiagnostic& cx_diagnostic);
  public:
    class FixIt {
    public:
      FixIt(const std::string &source, const std::pair<clang::Offset, clang::Offset> &offsets):
        source(source), offsets(offsets) {}
      std::string source;
      std::pair<clang::Offset, clang::Offset> offsets;
    };
    
    static const std::string get_severity_spelling(unsigned severity);

    unsigned severity;
    std::string severity_spelling;
    std::string spelling;
    std::string path;
    std::pair<clang::Offset, clang::Offset> offsets;
    std::vector<FixIt> fix_its;
  };
}

#endif  // DIAGNOSTIC_H_
