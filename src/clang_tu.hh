/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

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
