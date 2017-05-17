#include "TranslationUnit.h"

#include "Utility.h"
#include "../platform.h"

#include <fstream>
#include <sstream>
#include <cassert>
#include <iostream>

namespace clang {

TranslationUnit::TranslationUnit(Index& index,
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

  //std::cerr << "Parsing " << filepath << " with args " << StringJoin(args) << std::endl;

  //CXErrorCode error_code = clang_parseTranslationUnit2FullArgv(
  CXErrorCode error_code = clang_parseTranslationUnit2(
      index.cx_index, filepath.c_str(), args.data(), args.size(),
      unsaved_files.data(), unsaved_files.size(), flags, &cx_tu);

  switch (error_code) {
    case CXError_Success:
      did_fail = false;
      break;
    case CXError_Failure:
      std::cerr << "libclang generic failure for " << filepath << std::endl;
      did_fail = true;
      break;
    case CXError_Crashed:
      std::cerr << "libclang crashed for " << filepath << std::endl;
      did_fail = true;
      break;
    case CXError_InvalidArguments:
      std::cerr << "libclang had invalid arguments for " << filepath
                << std::endl;
      did_fail = true;
      break;
    case CXError_ASTReadError:
      std::cerr << "libclang had ast read error for " << filepath << std::endl;
      did_fail = true;
      break;
  }
}

TranslationUnit::~TranslationUnit() {
  clang_disposeTranslationUnit(cx_tu);
}

void TranslationUnit::ReparseTranslationUnit(
    std::vector<CXUnsavedFile>& unsaved) {
  int error_code =
      clang_reparseTranslationUnit(cx_tu, unsaved.size(), unsaved.data(),
                                   clang_defaultReparseOptions(cx_tu));
  switch (error_code) {
    case CXError_Success:
      did_fail = false;
      break;
    case CXError_Failure:
      std::cerr << "libclang reparse generic failure" << std::endl;
      did_fail = true;
      break;
    case CXError_Crashed:
      std::cerr << "libclang reparse crashed " << std::endl;
      did_fail = true;
      break;
    case CXError_InvalidArguments:
      std::cerr << "libclang reparse had invalid arguments" << std::endl;
      did_fail = true;
      break;
    case CXError_ASTReadError:
      std::cerr << "libclang reparse had ast read error" << std::endl;
      did_fail = true;
      break;
  }
}

Cursor TranslationUnit::document_cursor() const {
  return Cursor(clang_getTranslationUnitCursor(cx_tu));
}
}
