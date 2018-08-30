// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "position.h"

#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>

#include <stdlib.h>

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

std::unique_ptr<clang::CompilerInvocation>
BuildCompilerInvocation(const std::vector<std::string> &args,
                        llvm::IntrusiveRefCntPtr<clang::vfs::FileSystem> VFS);
