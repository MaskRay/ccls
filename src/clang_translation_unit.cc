#include "clang_translation_unit.h"

#include "clang_utils.h"
#include "platform.h"
#include "utils.h"

#include <loguru.hpp>

namespace {

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

  LOG_S(WARNING) << output;
}
}  // namespace

// static
std::unique_ptr<ClangTranslationUnit> ClangTranslationUnit::Create(
    ClangIndex* index,
    const std::string& filepath,
    const std::vector<std::string>& arguments,
    std::vector<CXUnsavedFile>& unsaved_files,
    unsigned flags) {
  std::vector<const char*> args;
  for (auto& arg : arguments)
    args.push_back(arg.c_str());

  CXTranslationUnit cx_tu;
  CXErrorCode error_code;
  {
    error_code = clang_parseTranslationUnit2FullArgv(
        index->cx_index, nullptr, args.data(), (int)args.size(),
        unsaved_files.data(), (unsigned)unsaved_files.size(), flags, &cx_tu);
  }

  if (error_code != CXError_Success && cx_tu)
    EmitDiagnostics(filepath, args, cx_tu);

  // We sometimes dump the command to logs and ask the user to run it. Include
  // -fsyntax-only so they don't do a full compile.
  auto make_msg = [&]() {
    return "Please try running the following, identify which flag causes the "
           "issue, and report a bug. cquery will then filter the flag for you "
           " automatically:\n$ " +
           StringJoin(args, " ") + " -fsyntax-only";
  };

  switch (error_code) {
    case CXError_Success:
      return std::make_unique<ClangTranslationUnit>(cx_tu);
    case CXError_Failure:
      LOG_S(ERROR) << "libclang generic failure for " << filepath << ". "
                   << make_msg();
      return nullptr;
    case CXError_Crashed:
      LOG_S(ERROR) << "libclang crashed for " << filepath << ". " << make_msg();
      return nullptr;
    case CXError_InvalidArguments:
      LOG_S(ERROR) << "libclang had invalid arguments for " << filepath << ". "
                   << make_msg();
      return nullptr;
    case CXError_ASTReadError:
      LOG_S(ERROR) << "libclang had ast read error for " << filepath << ". "
                   << make_msg();
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
