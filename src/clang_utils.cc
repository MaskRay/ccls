#include "clang_utils.h"

#include "libclangmm/Utility.h"
#include "platform.h"

namespace {

lsRange GetLsRangeForFixIt(const CXSourceRange& range) {
  CXSourceLocation start = clang_getRangeStart(range);
  CXSourceLocation end = clang_getRangeEnd(range);

  unsigned int start_line, start_column;
  clang_getSpellingLocation(start, nullptr, &start_line, &start_column, nullptr);
  unsigned int end_line, end_column;
  clang_getSpellingLocation(end, nullptr, &end_line, &end_column, nullptr);

  return lsRange(
    lsPosition(start_line - 1, start_column - 1) /*start*/,
    lsPosition(end_line - 1, end_column) /*end*/);
}

}  // namespace

optional<lsDiagnostic> BuildAndDisposeDiagnostic(
    CXDiagnostic diagnostic, const std::string& path) {
  // Get diagnostic location.
  CXFile file;
  unsigned int line, column;
  clang_getSpellingLocation(
      clang_getDiagnosticLocation(diagnostic), &file, &line, &column, nullptr);

  // Only report diagnostics in the same file. Using
  // clang_Location_isInSystemHeader causes crashes for some reason.
  if (path != FileName(file)) {
    clang_disposeDiagnostic(diagnostic);
    return nullopt;
  }

  // Build diagnostic.
  lsDiagnostic ls_diagnostic;

  // TODO: consider using clang_getDiagnosticRange
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

  // Report fixits
  unsigned num_fixits = clang_getDiagnosticNumFixIts(diagnostic);
  for (unsigned i = 0; i < num_fixits; ++i) {
    CXSourceRange replacement_range;
    CXString text = clang_getDiagnosticFixIt(diagnostic, i, &replacement_range);

    lsTextEdit edit;
    edit.newText = clang::ToString(text);
    edit.range = GetLsRangeForFixIt(replacement_range);
    ls_diagnostic.fixits_.push_back(edit);
  }

  clang_disposeDiagnostic(diagnostic);

  return ls_diagnostic;
}

std::string FileName(CXFile file) {
  CXString cx_name = clang_getFileName(file);
  std::string name = clang::ToString(cx_name);
  return NormalizePath(name);
}
