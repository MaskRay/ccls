#include "clang_tu.h"

#include "clang_utils.h"
#include "log.hh"
#include "platform.h"
#include "utils.h"
#include "working_files.h"

#include <llvm/Support/CrashRecoveryContext.h>
using namespace clang;

#include <assert.h>
#include <mutex>

Range FromSourceRange(const SourceManager &SM, const LangOptions &LangOpts,
                      SourceRange R, llvm::sys::fs::UniqueID *UniqueID,
                      bool token) {
  SourceLocation BLoc = R.getBegin(), ELoc = R.getEnd();
  std::pair<FileID, unsigned> BInfo = SM.getDecomposedLoc(BLoc);
  std::pair<FileID, unsigned> EInfo = SM.getDecomposedLoc(ELoc);
  if (token)
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
                    SourceRange R,
                    llvm::sys::fs::UniqueID *UniqueID) {
  return FromSourceRange(SM, LangOpts, R, UniqueID, false);
}

Range FromTokenRange(const SourceManager &SM, const LangOptions &LangOpts,
                     SourceRange R,
                     llvm::sys::fs::UniqueID *UniqueID) {
  return FromSourceRange(SM, LangOpts, R, UniqueID, true);
}

std::vector<ASTUnit::RemappedFile>
GetRemapped(const WorkingFiles::Snapshot &snapshot) {
  std::vector<ASTUnit::RemappedFile> Remapped;
  for (auto &file : snapshot.files) {
    std::unique_ptr<llvm::MemoryBuffer> MB =
        llvm::MemoryBuffer::getMemBufferCopy(file.content, file.filename);
    Remapped.emplace_back(file.filename, MB.release());
  }
  return Remapped;
}

std::unique_ptr<ClangTranslationUnit> ClangTranslationUnit::Create(
    const std::string &filepath, const std::vector<std::string> &args,
    const WorkingFiles::Snapshot &snapshot, bool diagnostic) {
  std::vector<const char *> Args;
  for (auto& arg : args)
    Args.push_back(arg.c_str());
  Args.push_back("-fno-spell-checking");
  Args.push_back("-fallow-editor-placeholders");

  auto ret = std::make_unique<ClangTranslationUnit>();
  IntrusiveRefCntPtr<DiagnosticsEngine> Diags(
      CompilerInstance::createDiagnostics(new DiagnosticOptions));
  std::vector<ASTUnit::RemappedFile> Remapped = GetRemapped(snapshot);

  ret->PCHCO = std::make_shared<PCHContainerOperations>();
  std::unique_ptr<ASTUnit> ErrUnit, Unit;
  llvm::CrashRecoveryContext CRC;
  auto parse = [&]() {
    Unit.reset(ASTUnit::LoadFromCommandLine(
        Args.data(), Args.data() + Args.size(),
        /*PCHContainerOpts=*/ret->PCHCO, Diags,
        /*ResourceFilePath=*/"", /*OnlyLocalDecls=*/false,
        /*CaptureDiagnostics=*/diagnostic, Remapped,
        /*RemappedFilesKeepOriginalName=*/true, 1,
        diagnostic ? TU_Complete : TU_Prefix,
        /*CacheCodeCompletionResults=*/true, g_config->index.comments,
        /*AllowPCHWithCompilerErrors=*/true,
#if LLVM_VERSION_MAJOR >= 7
        SkipFunctionBodiesScope::None,
#else
        false,
#endif
        /*SingleFileParse=*/false,
        /*UserFilesAreVolatile=*/true, false,
        ret->PCHCO->getRawReader().getFormat(), &ErrUnit));
  };
  if (!RunSafely(CRC, parse)) {
    LOG_S(ERROR)
        << "clang crashed for " << filepath << "\n"
        << StringJoin(args, " ") + " -fsyntax-only";
    return {};
  }
  if (!Unit && !ErrUnit)
    return {};

  ret->Unit = std::move(Unit);
  return ret;
}

int ClangTranslationUnit::Reparse(llvm::CrashRecoveryContext &CRC,
                                  const WorkingFiles::Snapshot &snapshot) {
  int ret = 1;
  auto parse = [&]() { ret = Unit->Reparse(PCHCO, GetRemapped(snapshot)); };
  (void)RunSafely(CRC, parse);
  return ret;
}
