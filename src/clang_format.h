#include <clang/Format/Format.h>

#include <vector>

struct ClangFormat {
  llvm::StringRef document_filename_;
  llvm::StringRef document_;
  llvm::ArrayRef<clang::tooling::Range> ranges_;
  int tab_size_;
  bool insert_spaces_;

  ClangFormat(llvm::StringRef document_filename,
              llvm::StringRef document,
              llvm::ArrayRef<clang::tooling::Range> ranges,
              int tab_size,
              bool insert_spaces);
  ~ClangFormat();

  std::vector<clang::tooling::Replacement> FormatWholeDocument();
};
