#if USE_CLANG_CXX

#include "clang_format.h"

#include <loguru.hpp>

using namespace clang::format;
using namespace clang::tooling;

ClangFormat::ClangFormat(llvm::StringRef document_filename,
                         llvm::StringRef document,
                         llvm::ArrayRef<clang::tooling::Range> ranges,
                         int tab_size,
                         bool insert_spaces)
    : document_filename_(document_filename),
      document_(document),
      ranges_(ranges),
      tab_size_(tab_size),
      insert_spaces_(insert_spaces) {}
ClangFormat::~ClangFormat(){};

static FormatStyle::LanguageKind getLanguageKindFromFilename(
    llvm::StringRef filename) {
  if (filename.endswith(".m") || filename.endswith(".mm")) {
    return FormatStyle::LK_ObjC;
  }
  return FormatStyle::LK_Cpp;
}

std::vector<Replacement> ClangFormat::FormatWholeDocument() {
  const auto language_kind = getLanguageKindFromFilename(document_filename_);
  FormatStyle predefined_style;
  getPredefinedStyle("chromium", language_kind, &predefined_style);
  llvm::Expected<FormatStyle> style =
      getStyle("file", document_filename_, "chromium");
  if (!style) {
    LOG_S(ERROR) << llvm::toString(style.takeError());
    return {};
  }

  if (*style == predefined_style) {
    style->UseTab = insert_spaces_ ? FormatStyle::UseTabStyle::UT_Never
                                  : FormatStyle::UseTabStyle::UT_Always;
    style->IndentWidth = tab_size_;
  }
  auto format_result = reformat(*style, document_, ranges_, document_filename_);
  return std::vector<Replacement>(format_result.begin(), format_result.end());
}

#endif
