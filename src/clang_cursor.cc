#include "clang_cursor.h"

#include "clang_utils.h"

#include <string.h>

#include <algorithm>
#include <cassert>

Range ResolveCXSourceRange(const CXSourceRange& range, CXFile* cx_file) {
  CXSourceLocation start = clang_getRangeStart(range);
  CXSourceLocation end = clang_getRangeEnd(range);

  unsigned int start_line, start_column;
  clang_getSpellingLocation(start, cx_file, &start_line, &start_column,
                            nullptr);
  unsigned int end_line, end_column;
  clang_getSpellingLocation(end, nullptr, &end_line, &end_column, nullptr);

  return Range(Position((int16_t)start_line - 1, (int16_t)start_column - 1),
               Position((int16_t)end_line - 1, (int16_t)end_column - 1));
}

ClangType::ClangType() : cx_type() {}

ClangType::ClangType(const CXType& other) : cx_type(other) {}

bool ClangType::operator==(const ClangType& rhs) const {
  return clang_equalTypes(cx_type, rhs.cx_type);
}

ClangCursor ClangType::get_declaration() const {
  return clang_getTypeDeclaration(cx_type);
}

std::string ClangType::get_usr() const {
  return ClangCursor(clang_getTypeDeclaration(cx_type)).get_usr();
}

Usr ClangType::get_usr_hash() const {
  if (is_builtin())
    return static_cast<Usr>(cx_type.kind);
  return ClangCursor(clang_getTypeDeclaration(cx_type)).get_usr_hash();
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
  return ClangType(clang_getResultType(cx_type));
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

static_assert(sizeof(ClangCursor) == sizeof(CXCursor),
              "Cursor must be the same size as CXCursor");

ClangCursor::ClangCursor() : cx_cursor(clang_getNullCursor()) {}

ClangCursor::ClangCursor(const CXCursor& other) : cx_cursor(other) {}

ClangCursor::operator bool() const {
  return !clang_Cursor_isNull(cx_cursor);
}

bool ClangCursor::operator==(const ClangCursor& rhs) const {
  return clang_equalCursors(cx_cursor, rhs.cx_cursor);
}

bool ClangCursor::operator!=(const ClangCursor& rhs) const {
  return !(*this == rhs);
}

CXCursorKind ClangCursor::get_kind() const {
  return cx_cursor.kind;
}

ClangType ClangCursor::get_type() const {
  return ClangType(clang_getCursorType(cx_cursor));
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
  return ClangCursor(clang_getCursorReferenced(cx_cursor));
}

ClangCursor ClangCursor::get_canonical() const {
  return ClangCursor(clang_getCanonicalCursor(cx_cursor));
}

ClangCursor ClangCursor::get_definition() const {
  return ClangCursor(clang_getCursorDefinition(cx_cursor));
}

ClangCursor ClangCursor::get_lexical_parent() const {
  return ClangCursor(clang_getCursorLexicalParent(cx_cursor));
}

ClangCursor ClangCursor::get_semantic_parent() const {
  return ClangCursor(clang_getCursorSemanticParent(cx_cursor));
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

NtString ClangCursor::get_comments() const {
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
  return static_cast<std::string_view>(ret);
}

std::string ClangCursor::ToString() const {
  return ::ToString(get_kind()) + " " + get_spell_name();
}
