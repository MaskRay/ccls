#if USE_CLANG_CXX

#include "clang_format.h"
#include "working_files.h"

#include <loguru.hpp>

using namespace clang;
using clang::format::FormatStyle;

namespace {

// TODO Objective-C 'header/interface' files may use .h, we should get this from
// project information.
FormatStyle::LanguageKind getLanguageKindFromFilename(
    llvm::StringRef filename) {
  if (filename.endswith(".m") || filename.endswith(".mm")) {
    return FormatStyle::LK_ObjC;
  }
  return FormatStyle::LK_Cpp;
}

}  // namespace

std::vector<tooling::Replacement> ClangFormatDocument(
    WorkingFile* working_file,
    int start,
    int end,
    lsFormattingOptions options) {
  const auto language_kind =
      getLanguageKindFromFilename(working_file->filename);
  FormatStyle predefined_style;
  getPredefinedStyle("chromium", language_kind, &predefined_style);
  llvm::Expected<FormatStyle> style =
      format::getStyle("file", working_file->filename, "chromium");
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
      llvm::ArrayRef<tooling::Range>(tooling::Range(start, end - start)),
      working_file->filename);
  return std::vector<tooling::Replacement>(format_result.begin(),
                                           format_result.end());
}

#endif
