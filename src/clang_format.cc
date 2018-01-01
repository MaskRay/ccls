#if USE_CLANG_CXX

#include "clang_format.h"
#include "working_files.h"

#include <loguru.hpp>

using namespace clang::format;
using namespace clang::tooling;

namespace {

FormatStyle::LanguageKind getLanguageKindFromFilename(
    llvm::StringRef filename) {
  if (filename.endswith(".m") || filename.endswith(".mm")) {
    return FormatStyle::LK_ObjC;
  }
  return FormatStyle::LK_Cpp;
}

}  // namespace

std::vector<Replacement> ClangFormatDocument(WorkingFile* working_file,
                                             int start,
                                             int end,
                                             lsFormattingOptions options) {
  const auto language_kind =
      getLanguageKindFromFilename(working_file->filename);
  FormatStyle predefined_style;
  getPredefinedStyle("chromium", language_kind, &predefined_style);
  llvm::Expected<FormatStyle> style =
      getStyle("file", working_file->filename, "chromium");
  if (!style) {
    // If, for some reason, we cannot get a format style, use Chromium's with
    // tab configuration provided by the client editor.
    LOG_S(ERROR) << llvm::toString(style.takeError());
    predefined_style.UseTab = options.insertSpaces
                                  ? FormatStyle::UseTabStyle::UT_Never
                                  : FormatStyle::UseTabStyle::UT_Always;
    predefined_style.IndentWidth = options.tabSize;
  }

  auto format_result = reformat(
      *style, working_file->buffer_content,
      llvm::ArrayRef<clang::tooling::Range>(clang::tooling::Range(start, end)),
      working_file->filename);
  return std::vector<Replacement>(format_result.begin(), format_result.end());
}

#endif
