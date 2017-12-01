#include "clang_translation_unit.h"

#include "clang_utils.h"
#include "platform.h"
#include "utils.h"

#include <loguru.hpp>

#include <cassert>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>

namespace {

// We need to serialize requests to clang_parseTranslationUnit2FullArgv and
// clang_reparseTranslationUnit. See
// https://github.com/jacobdufault/cquery/issues/43#issuecomment-347614504.
//
// NOTE: This is disabled because it effectively serializes indexing, as a huge
// chunk of indexing time is spent inside of these functions.
//
// std::mutex g_parse_translation_unit_mutex;
// std::mutex g_reparse_translation_unit_mutex;

void EmitDiagnostics(std::string path,
                     std::vector<const char*> args,
                     CXTranslationUnit tu) {
  std::string output = "Fatal errors while trying to parse " + path + "\n";
  output +=
      "Args: " +
      StringJoinMap(args, [](const char* arg) { return std::string(arg); }) +
      "\n";

  size_t num_diagnostics = clang_getNumDiagnostics(tu);
  for (unsigned i = 0; i < num_diagnostics; ++i) {
    output += "  - ";

    CXDiagnostic diagnostic = clang_getDiagnostic(tu, i);

    // Location.
    CXFile file;
    unsigned int line, column;
    clang_getSpellingLocation(clang_getDiagnosticLocation(diagnostic), &file,
                              &line, &column, nullptr);
    std::string path = FileName(file);
    output += path + ":" + std::to_string(line - 1) + ":" +
              std::to_string(column) + " ";

    // Severity
    switch (clang_getDiagnosticSeverity(diagnostic)) {
      case CXDiagnostic_Ignored:
      case CXDiagnostic_Note:
        output += "[info]";
        break;
      case CXDiagnostic_Warning:
        output += "[warning]";
        break;
      case CXDiagnostic_Error:
        output += "[error]";
        break;
      case CXDiagnostic_Fatal:
        output += "[fatal]";
        break;
    }

    // Content.
    output += " " + ToString(clang_getDiagnosticSpelling(diagnostic));

    clang_disposeDiagnostic(diagnostic);

    output += "\n";
  }

  std::cerr << output;
  std::cerr.flush();
}
}  // namespace

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
  CXErrorCode error_code;
  {
    // std::lock_guard<std::mutex> lock(g_parse_translation_unit_mutex);
    error_code = clang_parseTranslationUnit2FullArgv(
        index->cx_index, filepath.c_str(), args.data(), (int)args.size(),
        unsaved_files.data(), (unsigned)unsaved_files.size(), flags, &cx_tu);
  }

  if (error_code != CXError_Success && cx_tu)
    EmitDiagnostics(filepath, args, cx_tu);

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
  int error_code;
  {
    // std::lock_guard<std::mutex> lock(g_reparse_translation_unit_mutex);
    error_code = clang_reparseTranslationUnit(
        tu->cx_tu, (unsigned)unsaved.size(), unsaved.data(),
        clang_defaultReparseOptions(tu->cx_tu));
  }

  if (error_code != CXError_Success && tu->cx_tu)
    EmitDiagnostics("<unknown>", {}, tu->cx_tu);

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
