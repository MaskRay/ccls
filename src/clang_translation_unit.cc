#include "clang_translation_unit.h"

#include "platform.h"
#include "utils.h"

#include <loguru.hpp>

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

// static
std::unique_ptr<ClangTranslationUnit> ClangTranslationUnit::Create(
    ClangIndex* index,
    const std::string& filepath,
    const std::vector<std::string>& arguments,
    std::vector<CXUnsavedFile> unsaved_files,
    unsigned flags) {
  std::vector<const char*> args;
  for (const std::string& a : arguments)
    args.push_back(a.c_str());

  std::vector<std::string> platform_args = GetPlatformClangArguments();
  for (const auto& arg : platform_args)
    args.push_back(arg.c_str());

  CXTranslationUnit cx_tu;
  CXErrorCode error_code = clang_parseTranslationUnit2FullArgv(
      index->cx_index, nullptr, args.data(), (int)args.size(),
      unsaved_files.data(), (unsigned)unsaved_files.size(), flags, &cx_tu);

  switch (error_code) {
    case CXError_Success:
      return MakeUnique<ClangTranslationUnit>(cx_tu);
    case CXError_Failure:
      LOG_S(ERROR) << "libclang generic failure for " << filepath
                   << " with args " << StringJoin(args);
      return nullptr;
    case CXError_Crashed:
      LOG_S(ERROR) << "libclang crashed for " << filepath << " with args "
                   << StringJoin(args);
      return nullptr;
    case CXError_InvalidArguments:
      LOG_S(ERROR) << "libclang had invalid arguments for "
                   << " with args " << StringJoin(args) << filepath;
      return nullptr;
    case CXError_ASTReadError:
      LOG_S(ERROR) << "libclang had ast read error for " << filepath
                   << " with args " << StringJoin(args);
      return nullptr;
  }

  return nullptr;
}

// static
std::unique_ptr<ClangTranslationUnit> ClangTranslationUnit::Reparse(
    std::unique_ptr<ClangTranslationUnit> tu,
    std::vector<CXUnsavedFile>& unsaved) {
  int error_code = clang_reparseTranslationUnit(
      tu->cx_tu, (unsigned)unsaved.size(), unsaved.data(),
      clang_defaultReparseOptions(tu->cx_tu));
  switch (error_code) {
    case CXError_Success:
      return tu;
    case CXError_Failure:
      LOG_S(ERROR) << "libclang reparse generic failure";
      return nullptr;
    case CXError_Crashed:
      LOG_S(ERROR) << "libclang reparse crashed";
      return nullptr;
    case CXError_InvalidArguments:
      LOG_S(ERROR) << "libclang reparse had invalid arguments";
      return nullptr;
    case CXError_ASTReadError:
      LOG_S(ERROR) << "libclang reparse had ast read error";
      return nullptr;
  }

  return nullptr;
}

ClangTranslationUnit::ClangTranslationUnit(CXTranslationUnit tu) : cx_tu(tu) {}

ClangTranslationUnit::~ClangTranslationUnit() {
  clang_disposeTranslationUnit(cx_tu);
}
