// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "position.hh"

#include <clang/Basic/LangOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>

#if LLVM_VERSION_MAJOR < 8
// D52783 Lift VFS from clang to llvm
namespace llvm {
namespace vfs = clang::vfs;
}
#endif

namespace ccls {
std::string PathFromFileEntry(const clang::FileEntry &file);

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

Range FromTokenRangeDefaulted(const clang::SourceManager &SM,
                              const clang::LangOptions &Lang,
                              clang::SourceRange R, const clang::FileEntry *FE,
                              Range range);

std::unique_ptr<clang::CompilerInvocation>
BuildCompilerInvocation(std::vector<const char *> args,
                        llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> VFS);

const char *ClangBuiltinTypeName(int);
} // namespace ccls
