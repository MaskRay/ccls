#include "clang_format.h"

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

std::vector<Replacement> ClangFormat::FormatWholeDocument() {
  // TODO: Construct a valid style.
  FormatStyle style = getChromiumStyle(FormatStyle::LK_Cpp);
  style.UseTab = insert_spaces_ ? FormatStyle::UseTabStyle::UT_Never
                                : FormatStyle::UseTabStyle::UT_Always;
  style.IndentWidth = tab_size_;
  auto format_result = reformat(style, document_, ranges_, document_filename_);
  return std::vector<Replacement>(format_result.begin(), format_result.end());
}
