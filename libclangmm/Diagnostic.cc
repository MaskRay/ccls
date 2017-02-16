#include "Diagnostic.h"
#include "SourceLocation.h"
#include "Tokens.h"
#include "Utility.h"

clang::Diagnostic::Diagnostic(CXTranslationUnit& cx_tu, CXDiagnostic& cx_diagnostic) {
  severity=clang_getDiagnosticSeverity(cx_diagnostic);
  severity_spelling=get_severity_spelling(severity);
  spelling=to_string(clang_getDiagnosticSpelling(cx_diagnostic));
  
  SourceLocation start_location(clang_getDiagnosticLocation(cx_diagnostic));
  path=start_location.get_path();
  auto start_offset=start_location.get_offset();
  Tokens tokens(cx_tu, SourceRange(start_location, start_location));
  if(tokens.size()==1)
    offsets={start_offset, tokens.begin()->offsets.second};
  
  unsigned num_fix_its=clang_getDiagnosticNumFixIts(cx_diagnostic);
  for(unsigned c=0;c<num_fix_its;c++) {
    CXSourceRange fix_it_range;
    auto source=to_string(clang_getDiagnosticFixIt(cx_diagnostic, c, &fix_it_range));
    fix_its.emplace_back(source, SourceRange(fix_it_range).get_offsets());
  }
}

const std::string clang::Diagnostic::get_severity_spelling(unsigned severity) {
  switch(severity) {
    case CXDiagnostic_Ignored:
      return "Ignored";
    case CXDiagnostic_Note:
      return "Note";
    case CXDiagnostic_Warning:
      return "Warning";
    case CXDiagnostic_Error:
      return "Error";
    case CXDiagnostic_Fatal:
      return "Fatal";
    default:
      return "";
  }
}
