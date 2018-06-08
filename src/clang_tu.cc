#include "clang_tu.h"

#include "clang_utils.h"
#include "log.hh"
#include "platform.h"
#include "utils.h"

#include <assert.h>
#include <string.h>
#include <algorithm>
#include <mutex>

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

Range ResolveCXSourceRange(const CXSourceRange& range, CXFile* cx_file) {
  CXSourceLocation start = clang_getRangeStart(range);
  CXSourceLocation end = clang_getRangeEnd(range);

  unsigned int start_line, start_column;
  clang_getSpellingLocation(start, cx_file, &start_line, &start_column,
                            nullptr);
  unsigned int end_line, end_column;
  clang_getSpellingLocation(end, nullptr, &end_line, &end_column, nullptr);

  return Range{{int16_t(start_line - 1), (int16_t)(start_column - 1)},
               {int16_t(end_line - 1), int16_t(end_column - 1)}};
}

ClangCursor ClangType::get_declaration() const {
  return clang_getTypeDeclaration(cx_type);
}

std::string ClangType::get_usr() const {
  return ClangCursor{clang_getTypeDeclaration(cx_type)}.get_usr();
}

Usr ClangType::get_usr_hash() const {
  if (is_builtin())
    return static_cast<Usr>(cx_type.kind);
  return ClangCursor{clang_getTypeDeclaration(cx_type)}.get_usr_hash();
}

ClangType ClangType::get_canonical() const {
  return clang_getCanonicalType(cx_type);
}

ClangType ClangType::strip_qualifiers() const {
  CXType cx = cx_type;
  while (1) {
    switch (cx.kind) {
      default:
        break;
      case CXType_ConstantArray:
      case CXType_DependentSizedArray:
      case CXType_IncompleteArray:
      case CXType_VariableArray:
        cx = clang_getElementType(cx);
        continue;
      case CXType_BlockPointer:
      case CXType_LValueReference:
      case CXType_MemberPointer:
      case CXType_ObjCObjectPointer:
      case CXType_Pointer:
      case CXType_RValueReference:
        cx = clang_getPointeeType(cx);
        continue;
    }
    break;
  }

  return cx;
}

std::string ClangType::get_spell_name() const {
  return ToString(clang_getTypeSpelling(cx_type));
}

ClangType ClangType::get_return_type() const {
  return clang_getResultType(cx_type);
}

std::vector<ClangType> ClangType::get_arguments() const {
  int size = clang_getNumArgTypes(cx_type);
  if (size < 0)
    return {};
  std::vector<ClangType> types(size);
  for (int i = 0; i < size; ++i)
    types.emplace_back(clang_getArgType(cx_type, i));
  return types;
}

std::vector<ClangType> ClangType::get_template_arguments() const {
  int size = clang_Type_getNumTemplateArguments(cx_type);
  assert(size >= 0);
  if (size < 0)
    return std::vector<ClangType>();

  std::vector<ClangType> types(size);
  for (int i = 0; i < size; ++i)
    types.emplace_back(clang_Type_getTemplateArgumentAsType(cx_type, i));
  return types;
}

ClangType ClangCursor::get_type() const {
  return {clang_getCursorType(cx_cursor)};
}

std::string ClangCursor::get_spell_name() const {
  return ::ToString(clang_getCursorSpelling(cx_cursor));
}

Range ClangCursor::get_spell(CXFile* cx_file) const {
  // TODO for Objective-C methods and Objective-C message expressions, there are
  // multiple pieces for each selector identifier.
  CXSourceRange range = clang_Cursor_getSpellingNameRange(cx_cursor, 0, 0);
  return ResolveCXSourceRange(range, cx_file);
}

Range ClangCursor::get_extent() const {
  CXSourceRange range = clang_getCursorExtent(cx_cursor);
  return ResolveCXSourceRange(range, nullptr);
}

std::string ClangCursor::get_display_name() const {
  return ::ToString(clang_getCursorDisplayName(cx_cursor));
}

std::string ClangCursor::get_usr() const {
  return ::ToString(clang_getCursorUSR(cx_cursor));
}

Usr ClangCursor::get_usr_hash() const {
  CXString usr = clang_getCursorUSR(cx_cursor);
  Usr ret = HashUsr(clang_getCString(usr));
  clang_disposeString(usr);
  return ret;
}

std::optional<Usr> ClangCursor::get_opt_usr_hash() const {
  CXString usr = clang_getCursorUSR(cx_cursor);
  const char* str = clang_getCString(usr);
  if (!str || str[0] == '\0') {
    clang_disposeString(usr);
    return {};
  }
  Usr ret = HashUsr(str);
  clang_disposeString(usr);
  return ret;
}

bool ClangCursor::is_definition() const {
  return clang_isCursorDefinition(cx_cursor);
}

ClangCursor ClangCursor::template_specialization_to_template_definition()
    const {
  CXCursor definition = clang_getSpecializedCursorTemplate(cx_cursor);
  if (definition.kind == CXCursor_FirstInvalid)
    return cx_cursor;
  return definition;
}

ClangCursor ClangCursor::get_referenced() const {
  return {clang_getCursorReferenced(cx_cursor)};
}

ClangCursor ClangCursor::get_canonical() const {
  return {clang_getCanonicalCursor(cx_cursor)};
}

ClangCursor ClangCursor::get_definition() const {
  return {clang_getCursorDefinition(cx_cursor)};
}

ClangCursor ClangCursor::get_lexical_parent() const {
  return {clang_getCursorLexicalParent(cx_cursor)};
}

ClangCursor ClangCursor::get_semantic_parent() const {
  return {clang_getCursorSemanticParent(cx_cursor)};
}

std::vector<ClangCursor> ClangCursor::get_arguments() const {
  int size = clang_Cursor_getNumArguments(cx_cursor);
  if (size < 0)
    return std::vector<ClangCursor>();

  std::vector<ClangCursor> cursors(size);
  for (int i = 0; i < size; ++i)
    cursors.emplace_back(clang_Cursor_getArgument(cx_cursor, i));
  return cursors;
}

bool ClangCursor::is_valid_kind() const {
  CXCursor referenced = clang_getCursorReferenced(cx_cursor);
  if (clang_Cursor_isNull(referenced))
    return false;

  CXCursorKind kind = get_kind();
  return kind > CXCursor_UnexposedDecl &&
         (kind < CXCursor_FirstInvalid || kind > CXCursor_LastInvalid);
}

std::string ClangCursor::get_type_description() const {
  auto type = clang_getCursorType(cx_cursor);
  return ::ToString(clang_getTypeSpelling(type));
}

std::string ClangCursor::get_comments() const {
  CXSourceRange range = clang_Cursor_getCommentRange(cx_cursor);
  if (clang_Range_isNull(range))
    return {};

  unsigned start_column;
  clang_getSpellingLocation(clang_getRangeStart(range), nullptr, nullptr,
                            &start_column, nullptr);

  // Get associated comment text.
  CXString cx_raw = clang_Cursor_getRawCommentText(cx_cursor);
  int pad = -1;
  std::string ret;
  for (const char* p = clang_getCString(cx_raw); *p;) {
    // The first line starts with a comment marker, but the rest needs
    // un-indenting.
    unsigned skip = start_column - 1;
    for (; skip > 0 && (*p == ' ' || *p == '\t'); p++)
      skip--;
    const char* q = p;
    while (*q != '\n' && *q)
      q++;
    if (*q)
      q++;
    // A minimalist approach to skip Doxygen comment markers.
    // See https://www.stack.nl/~dimitri/doxygen/manual/docblocks.html
    if (pad < 0) {
      // First line, detect the length of comment marker and put into |pad|
      const char* begin = p;
      while (*p == '/' || *p == '*')
        p++;
      if (*p == '<' || *p == '!')
        p++;
      if (*p == ' ')
        p++;
      pad = int(p - begin);
    } else {
      // Other lines, skip |pad| bytes
      int prefix = pad;
      while (prefix > 0 &&
             (*p == ' ' || *p == '/' || *p == '*' || *p == '<' || *p == '!'))
        prefix--, p++;
    }
    ret.insert(ret.end(), p, q);
    p = q;
  }
  clang_disposeString(cx_raw);
  while (ret.size() && isspace(ret.back()))
    ret.pop_back();
  if (EndsWith(ret, "*/")) {
    ret.resize(ret.size() - 2);
  } else if (EndsWith(ret, "\n/")) {
    ret.resize(ret.size() - 2);
  }
  while (ret.size() && isspace(ret.back()))
    ret.pop_back();
  if (ret.empty())
    return {};
  return ret;
}

std::string ClangCursor::ToString() const {
  return ::ToString(get_kind()) + " " + get_spell_name();
}

ClangIndex::ClangIndex() : ClangIndex(1, 0) {}

ClangIndex::ClangIndex(int exclude_declarations_from_pch,
                       int display_diagnostics) {
  // llvm::InitializeAllTargets (and possibly others) called by
  // clang_createIndex transtively modifies/reads lib/Support/TargetRegistry.cpp
  // FirstTarget. There will be a race condition if two threads call
  // clang_createIndex concurrently.
  static std::mutex mutex_;
  std::lock_guard<std::mutex> lock(mutex_);

  cx_index =
      clang_createIndex(exclude_declarations_from_pch, display_diagnostics);
}

ClangIndex::~ClangIndex() {
  clang_disposeIndex(cx_index);
}

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
           "issue, and report a bug. ccls will then filter the flag for you "
           " automatically:\n " +
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
