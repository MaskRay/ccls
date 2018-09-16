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

#include "clang_tu.h"

#include "clang_utils.h"
#include "config.h"

#include <clang/Lex/Lexer.h>
using namespace clang;

#include <assert.h>
#include <mutex>

Range FromCharSourceRange(const SourceManager &SM, const LangOptions &LangOpts,
                          CharSourceRange R,
                          llvm::sys::fs::UniqueID *UniqueID) {
  SourceLocation BLoc = R.getBegin(), ELoc = R.getEnd();
  std::pair<FileID, unsigned> BInfo = SM.getDecomposedLoc(BLoc);
  std::pair<FileID, unsigned> EInfo = SM.getDecomposedLoc(ELoc);
  if (R.isTokenRange())
    EInfo.second += Lexer::MeasureTokenLength(ELoc, SM, LangOpts);
  unsigned l0 = SM.getLineNumber(BInfo.first, BInfo.second) - 1,
           c0 = SM.getColumnNumber(BInfo.first, BInfo.second) - 1,
           l1 = SM.getLineNumber(EInfo.first, EInfo.second) - 1,
           c1 = SM.getColumnNumber(EInfo.first, EInfo.second) - 1;
  if (l0 > INT16_MAX)
    l0 = 0;
  if (c0 > INT16_MAX)
    c0 = 0;
  if (l1 > INT16_MAX)
    l1 = 0;
  if (c1 > INT16_MAX)
    c1 = 0;
  if (UniqueID) {
    if (const FileEntry *F = SM.getFileEntryForID(BInfo.first))
      *UniqueID = F->getUniqueID();
    else
      *UniqueID = llvm::sys::fs::UniqueID(0, 0);
  }
  return {{int16_t(l0), int16_t(c0)}, {int16_t(l1), int16_t(c1)}};
}

Range FromCharRange(const SourceManager &SM, const LangOptions &LangOpts,
                    SourceRange R, llvm::sys::fs::UniqueID *UniqueID) {
  return FromCharSourceRange(SM, LangOpts, CharSourceRange::getCharRange(R),
                             UniqueID);
}

Range FromTokenRange(const SourceManager &SM, const LangOptions &LangOpts,
                     SourceRange R, llvm::sys::fs::UniqueID *UniqueID) {
  return FromCharSourceRange(SM, LangOpts, CharSourceRange::getTokenRange(R),
                             UniqueID);
}

std::unique_ptr<CompilerInvocation>
BuildCompilerInvocation(const std::vector<std::string> &args,
                        IntrusiveRefCntPtr<vfs::FileSystem> VFS) {
  std::string save = "-resource-dir=" + g_config->clang.resourceDir;
  std::vector<const char *> cargs;
  for (auto &arg : args)
    cargs.push_back(arg.c_str());
  cargs.push_back(save.c_str());
  IntrusiveRefCntPtr<DiagnosticsEngine> Diags(
      CompilerInstance::createDiagnostics(new DiagnosticOptions));
  std::unique_ptr<CompilerInvocation> CI =
      createInvocationFromCommandLine(cargs, Diags, VFS);
  if (CI) {
    CI->getDiagnosticOpts().IgnoreWarnings = true;
    CI->getFrontendOpts().DisableFree = false;
    CI->getLangOpts()->SpellChecking = false;
  }
  return CI;
}
