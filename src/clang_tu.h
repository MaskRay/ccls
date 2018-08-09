#pragma once
#include "position.h"
#include "working_files.h"

#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <llvm/Support/CrashRecoveryContext.h>

#include <memory>
#include <stdlib.h>
#include <string>
#include <vector>

std::vector<clang::ASTUnit::RemappedFile>
GetRemapped(const WorkingFiles::Snapshot &snapshot);

Range FromCharSourceRange(const clang::SourceManager &SM,
                          const clang::LangOptions &LangOpts,
                          clang::CharSourceRange R,
                          llvm::sys::fs::UniqueID *UniqueID = nullptr);

Range FromCharRange(const clang::SourceManager &SM,
                    const clang::LangOptions &LangOpts, clang::SourceRange R,
                    llvm::sys::fs::UniqueID *UniqueID = nullptr);

Range FromTokenRange(const clang::SourceManager &SM,
                     const clang::LangOptions &LangOpts, clang::SourceRange R,
                     llvm::sys::fs::UniqueID *UniqueID = nullptr);

struct ClangTranslationUnit {
  static std::unique_ptr<ClangTranslationUnit>
  Create(const std::string &filepath, const std::vector<std::string> &args,
         const WorkingFiles::Snapshot &snapshot, bool diagnostic);

  int Reparse(llvm::CrashRecoveryContext &CRC,
              const WorkingFiles::Snapshot &snapshot);

  std::shared_ptr<clang::PCHContainerOperations> PCHCO;
  std::unique_ptr<clang::ASTUnit> Unit;
};
