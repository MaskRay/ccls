// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "position.hh"

#include <clang/Basic/FileManager.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>

#if LLVM_VERSION_MAJOR < 8
// D52783 Lift VFS from clang to llvm
namespace llvm {
namespace vfs = clang::vfs;
}
#endif

namespace ccls {
std::string pathFromFileEntry(const clang::FileEntry &file);

bool isInsideMainFile(const clang::SourceManager &sm, clang::SourceLocation sl);

Range fromCharSourceRange(const clang::SourceManager &sm,
                          const clang::LangOptions &lang,
                          clang::CharSourceRange csr,
                          clang::FileID *fid = nullptr);

Range fromTokenRange(const clang::SourceManager &sm,
                     const clang::LangOptions &lang, clang::SourceRange sr,
                     clang::FileID *fid = nullptr);

Range fromTokenRangeDefaulted(const clang::SourceManager &sm,
                              const clang::LangOptions &lang,
                              clang::SourceRange sr, clang::FileID fid,
                              Range range);

std::unique_ptr<clang::CompilerInvocation>
buildCompilerInvocation(const std::string &main, std::vector<const char *> args,
                        llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> VFS);

const char *clangBuiltinTypeName(int);
} // namespace ccls
