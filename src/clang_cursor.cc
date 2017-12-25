#include "clang_cursor.h"

#include "clang_utils.h"

#include <algorithm>
#include <cassert>

// TODO Place this global variable into config
bool g_enable_comments = false;

ClangType::ClangType() : cx_type() {}

ClangType::ClangType(const CXType& other) : cx_type(other) {}

bool ClangType::operator==(const ClangType& rhs) const {
  return clang_equalTypes(cx_type, rhs.cx_type);
}

bool ClangType::is_fundamental() const {
  // NOTE: This will return false for pointed types. Should we call
  //       strip_qualifiers for the user?
  return cx_type.kind >= CXType_FirstBuiltin &&
         cx_type.kind <= CXType_LastBuiltin;
}

CXCursor ClangType::get_declaration() const {
  return clang_getTypeDeclaration(cx_type);
}

std::string ClangType::get_usr() const {
  return ClangCursor(clang_getTypeDeclaration(cx_type)).get_usr();
}

ClangType ClangType::get_canonical() const {
  return clang_getCanonicalType(cx_type);
}

ClangType ClangType::strip_qualifiers() const {
  // CXRefQualifierKind qualifiers = clang_Type_getCXXRefQualifier(cx_type)
  switch (cx_type.kind) {
    case CXType_LValueReference:
    case CXType_Pointer:
      return clang_getPointeeType(cx_type);
    default:
      break;
  }

  return cx_type;
}

std::string ClangType::get_spelling() const {
  return ToString(clang_getTypeSpelling(cx_type));
}

ClangType ClangType::get_return_type() const {
  return ClangType(clang_getResultType(cx_type));
}

std::vector<ClangType> ClangType::get_arguments() const {
  int size = clang_getNumArgTypes(cx_type);
  assert(size >= 0);
  if (size < 0)
    return std::vector<ClangType>();

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

ClangCursor ClangCursor::get_declaration() const {
  ClangType type = get_type();

  // auto x = new Foo() will not be deduced to |Foo| if we do not use the
  // canonical type. However, a canonical type will look past typedefs so we
  // will not accurately report variables on typedefs if we always do this.
  if (type.cx_type.kind == CXType_Auto)
    type = type.get_canonical();

  return type.strip_qualifiers().get_declaration();
}

ClangType ClangCursor::get_type() const {
  return ClangType(clang_getCursorType(cx_cursor));
}

std::string ClangCursor::get_spelling() const {
  return ::ToString(clang_getCursorSpelling(cx_cursor));
}

std::string ClangCursor::get_display_name() const {
  return ::ToString(clang_getCursorDisplayName(cx_cursor));
}

std::string ClangCursor::get_usr() const {
  return ::ToString(clang_getCursorUSR(cx_cursor));
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

optional<std::string> ClangCursor::get_comments() const {
  if (!g_enable_comments)
    return nullopt;
  CXSourceRange range = clang_Cursor_getCommentRange(cx_cursor);
  if (clang_Range_isNull(range))
    return nullopt;

  unsigned start_column;
  clang_getSpellingLocation(clang_getRangeStart(range), nullptr, nullptr,
                            &start_column, nullptr);

  // Get associated comment text.
  CXString cx_raw = clang_Cursor_getRawCommentText(cx_cursor);
  // The first line starts with a comment marker, but the rest needs un-indenting.
  std::string unindented;
  for (const char* p = clang_getCString(cx_raw); *p; ) {
    auto skip = start_column - 1;
    for (; skip > 0 && (*p == ' ' || *p == '\t'); p++)
      skip--;
    const char* q = p;
    while (*q != '\n' && *q) q++;
    if (*q) q++;
    unindented.insert(unindented.end(), p, q);
    p = q;
  }
  clang_disposeString(cx_raw);
  return unindented;
}

std::string ClangCursor::ToString() const {
  return ::ToString(get_kind()) + " " + get_spelling();
}
