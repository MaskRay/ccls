#include "clang_utils.h"

#include "libclangmm/Utility.h"

optional<lsDiagnostic> BuildDiagnostic(CXDiagnostic diagnostic) {
  // Skip diagnostics in system headers.
  CXSourceLocation diag_loc = clang_getDiagnosticLocation(diagnostic);
  if (clang_Location_isInSystemHeader(diag_loc))
    return nullopt;

  // Get db so we can attribute diagnostic to the right indexed file.
  CXFile file;
  unsigned int line, column;
  clang_getSpellingLocation(diag_loc, &file, &line, &column, nullptr);

  // Build diagnostic.

  lsDiagnostic ls_diagnostic;

  // TODO: ls_diagnostic.range is lsRange, we have Range. We should only be
  // storing Range types when inside the indexer so that index <-> buffer
  // remapping logic is applied.
  ls_diagnostic.range = lsRange(lsPosition(line - 1, column), lsPosition(line - 1, column));

  ls_diagnostic.message = clang::ToString(clang_getDiagnosticSpelling(diagnostic));

  // Append the flag that enables this diagnostic, ie, [-Wswitch]
  std::string enabling_flag = clang::ToString(clang_getDiagnosticOption(diagnostic, nullptr));
  if (!enabling_flag.empty())
    ls_diagnostic.message += " [" + enabling_flag + "]";

  ls_diagnostic.code = clang_getDiagnosticCategory(diagnostic);

  switch (clang_getDiagnosticSeverity(diagnostic)) {
  case CXDiagnostic_Ignored:
  case CXDiagnostic_Note:
    ls_diagnostic.severity = lsDiagnosticSeverity::Information;
    break;
  case CXDiagnostic_Warning:
    ls_diagnostic.severity = lsDiagnosticSeverity::Warning;
    break;
  case CXDiagnostic_Error:
  case CXDiagnostic_Fatal:
    ls_diagnostic.severity = lsDiagnosticSeverity::Error;
    break;
  }

  // TODO: integrate FixIts (ie, textDocument/codeAction)

  clang_disposeDiagnostic(diagnostic);

  return ls_diagnostic;
}
