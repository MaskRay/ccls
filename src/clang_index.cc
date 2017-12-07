#include "clang_index.h"

#include <mutex>

ClangIndex::ClangIndex() : ClangIndex(1, 0) {}

ClangIndex::ClangIndex(int exclude_declarations_from_pch,
                       int display_diagnostics) {
  // llvm::InitializeAllTargets (and possibly others) called by
  // clang_createIndex transtively modifies/reads lib/Support/TargetRegistry.cpp
  // FirstTarget. There will be a race condition if two threads call
  // clang_createIndex concurrently.
  static std::mutex mutex_;
  std::lock_guard<std::mutex> lock(mutex_);

  cx_index =
      clang_createIndex(exclude_declarations_from_pch, display_diagnostics);
}

ClangIndex::~ClangIndex() {
  clang_disposeIndex(cx_index);
}
