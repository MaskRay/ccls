#if USE_CLANG_CXX

#include "clang_format.h"
#include "working_files.h"

#include <doctest/doctest.h>
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
      style ? *style : predefined_style, working_file->buffer_content,
      llvm::ArrayRef<tooling::Range>(tooling::Range(start, end - start)),
      working_file->filename);
  return std::vector<tooling::Replacement>(format_result.begin(),
                                           format_result.end());
}

TEST_SUITE("ClangFormat") {
  TEST_CASE("entireDocument") {
    const std::string sample_document = "int main() { int *i = 0; return 0; }";
    WorkingFile* file = new WorkingFile("foo.cc", sample_document);
    lsFormattingOptions formatting_options;
    formatting_options.insertSpaces = true;
    const auto replacements = ClangFormatDocument(
        file, 0, sample_document.size(), formatting_options);

    // echo "int main() { int *i = 0; return 0; }" | clang-format
    // -style=Chromium -output-replacements-xml
    //
    // <?xml version='1.0'?>
    // <replacements xml:space='preserve' incomplete_format='false'>
    // <replacement offset='12' length='1'>&#10;  </replacement>
    // <replacement offset='16' length='1'></replacement>
    // <replacement offset='18' length='0'> </replacement>
    // <replacement offset='24' length='1'>&#10;  </replacement>
    // <replacement offset='34' length='1'>&#10;</replacement>
    // </replacements>

    REQUIRE(replacements.size() == 5);
    REQUIRE(replacements[0].getOffset() == 12);
    REQUIRE(replacements[0].getLength() == 1);
    REQUIRE(replacements[0].getReplacementText() == "\n  ");

    REQUIRE(replacements[1].getOffset() == 16);
    REQUIRE(replacements[1].getLength() == 1);
    REQUIRE(replacements[1].getReplacementText() == "");

    REQUIRE(replacements[2].getOffset() == 18);
    REQUIRE(replacements[2].getLength() == 0);
    REQUIRE(replacements[2].getReplacementText() == " ");

    REQUIRE(replacements[3].getOffset() == 24);
    REQUIRE(replacements[3].getLength() == 1);
    REQUIRE(replacements[3].getReplacementText() == "\n  ");

    REQUIRE(replacements[4].getOffset() == 34);
    REQUIRE(replacements[4].getLength() == 1);
    REQUIRE(replacements[4].getReplacementText() == "\n");
  }

  TEST_CASE("range") {
    const std::string sampleDocument = "int main() { int *i = 0; return 0; }";
    WorkingFile* file = new WorkingFile("foo.cc", sampleDocument);
    lsFormattingOptions formattingOptions;
    formattingOptions.insertSpaces = true;
    const auto replacements =
        ClangFormatDocument(file, 30, sampleDocument.size(), formattingOptions);

    REQUIRE(replacements.size() == 2);
    REQUIRE(replacements[0].getOffset() == 24);
    REQUIRE(replacements[0].getLength() == 1);
    REQUIRE(replacements[0].getReplacementText() == "\n  ");

    REQUIRE(replacements[1].getOffset() == 34);
    REQUIRE(replacements[1].getLength() == 1);
    REQUIRE(replacements[1].getReplacementText() == "\n");
  }
}

#endif
