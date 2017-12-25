#include "clang_utils.h"

#include "platform.h"

namespace {

lsRange GetLsRangeForFixIt(const CXSourceRange& range) {
  CXSourceLocation start = clang_getRangeStart(range);
  CXSourceLocation end = clang_getRangeEnd(range);

  unsigned int start_line, start_column;
  clang_getSpellingLocation(start, nullptr, &start_line, &start_column,
                            nullptr);
  unsigned int end_line, end_column;
  clang_getSpellingLocation(end, nullptr, &end_line, &end_column, nullptr);

  return lsRange(lsPosition(start_line - 1, start_column - 1) /*start*/,
                 lsPosition(end_line - 1, end_column) /*end*/);
}

}  // namespace

// See clang_formatDiagnostic
optional<lsDiagnostic> BuildAndDisposeDiagnostic(CXDiagnostic diagnostic,
                                                 const std::string& path) {
  // Get diagnostic location.
  CXFile file;
  unsigned start_line, start_column;
  clang_getSpellingLocation(clang_getDiagnosticLocation(diagnostic), &file,
                            &start_line, &start_column, nullptr);

  if (file && path != FileName(file)) {
    clang_disposeDiagnostic(diagnostic);
    return nullopt;
  }

  unsigned end_line = start_line, end_column = start_column,
           num_ranges = clang_getDiagnosticNumRanges(diagnostic);
  for (unsigned i = 0; i < num_ranges; i++) {
    CXFile file0, file1;
    unsigned line0, column0, line1, column1;
    CXSourceRange range = clang_getDiagnosticRange(diagnostic, i);
    clang_getSpellingLocation(clang_getRangeStart(range), &file0, &line0,
                              &column0, nullptr);
    clang_getSpellingLocation(clang_getRangeEnd(range), &file1, &line1,
                              &column1, nullptr);
    if (file0 != file1 || file0 != file)
      continue;
    if (line0 < start_line || (line0 == start_line && column0 < start_column)) {
      start_line = line0;
      start_column = column0;
    }
    if (line1 > end_line || (line1 == end_line && column1 > end_column)) {
      end_line = line1;
      end_column = column1;
    }
  }

  // Build diagnostic.
  lsDiagnostic ls_diagnostic;
  ls_diagnostic.range = lsRange(lsPosition(start_line - 1, start_column - 1),
                                lsPosition(end_line - 1, end_column - 1));

  ls_diagnostic.message = ToString(clang_getDiagnosticSpelling(diagnostic));

  // Append the flag that enables this diagnostic, ie, [-Wswitch]
  std::string enabling_flag =
      ToString(clang_getDiagnosticOption(diagnostic, nullptr));
  if (!enabling_flag.empty())
    ls_diagnostic.message += " [" + enabling_flag + "]";

  ls_diagnostic.code = clang_getDiagnosticCategory(diagnostic);

  switch (clang_getDiagnosticSeverity(diagnostic)) {
    case CXDiagnostic_Ignored:
      // llvm_unreachable
      break;
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
    edit.newText = ToString(text);
    edit.range = GetLsRangeForFixIt(replacement_range);
    ls_diagnostic.fixits_.push_back(edit);
  }

  clang_disposeDiagnostic(diagnostic);

  return ls_diagnostic;
}

static lsPosition OffsetToRange(llvm::StringRef document, size_t offset) {
  // TODO: Support Windows line endings, etc.
  llvm::StringRef text_before = document.substr(0, offset);
  int num_line = text_before.count('\n');
  int num_column = text_before.size() - text_before.rfind('\n') - 1;
  return {num_line, num_column};
}

std::vector<lsTextEdit> ConvertClangReplacementsIntoTextEdits(
    llvm::StringRef document,
    const std::vector<clang::tooling::Replacement>& clang_replacements) {
  std::vector<lsTextEdit> text_edits_result;
  for (const auto& replacement : clang_replacements) {
    const auto startPosition = OffsetToRange(document, replacement.getOffset());
    const auto endPosition = OffsetToRange(
        document, replacement.getOffset() + replacement.getLength());
    text_edits_result.push_back(
        {{startPosition, endPosition}, replacement.getReplacementText()});
  }
  return text_edits_result;
}

std::string FileName(CXFile file) {
  CXString cx_name = clang_getFileName(file);
  std::string name = ToString(cx_name);
  return NormalizePath(name);
}

std::string ToString(CXString cx_string) {
  std::string string;
  if (cx_string.data != nullptr) {
    string = clang_getCString(cx_string);
    clang_disposeString(cx_string);
  }
  return string;
}

std::string ToString(CXCursorKind kind) {
  return ToString(clang_getCursorKindSpelling(kind));
}
