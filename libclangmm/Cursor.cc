#include <algorithm>
#include <cassert>

#include "Cursor.h"
#include "Utility.h"

namespace clang {

Type::Type() : cx_type() {}

Type::Type(const CXType& other) : cx_type(other) {}


bool Type::operator==(const Type& rhs) const {
  return clang_equalTypes(cx_type, rhs.cx_type);
}

std::string Type::get_usr() const {
  return clang::Cursor(clang_getTypeDeclaration(cx_type)).get_usr();
}

std::string Type::get_spelling() const {
  return ToString(clang_getTypeSpelling(cx_type));
}

Type Type::get_return_type() const {
  return Type(clang_getResultType(cx_type));
}

std::vector<Type> Type::get_arguments() const {
  int size = clang_getNumArgTypes(cx_type);
  assert(size >= 0);
  if (size < 0)
    return std::vector<Type>();

  std::vector<Type> types(size);
  for (int i = 0; i < size; ++i)
    types.emplace_back(clang_getArgType(cx_type, i));
  return types;
}



static_assert(sizeof(Cursor) == sizeof(CXCursor),
  "Cursor must be the same size as CXCursor");

Cursor::Cursor() : cx_cursor(clang_getNullCursor()) {}

Cursor::Cursor(const CXCursor& other) : cx_cursor(other) {}

Cursor::operator bool() const {
  return !clang_Cursor_isNull(cx_cursor);
}

bool Cursor::operator==(const Cursor& rhs) const {
  return clang_equalCursors(cx_cursor, rhs.cx_cursor);
}

CXCursorKind Cursor::get_kind() const {
  return cx_cursor.kind;
}

Type Cursor::get_type() const {
  return Type(clang_getCursorType(cx_cursor));
}

SourceLocation Cursor::get_source_location() const {
  return SourceLocation(clang_getCursorLocation(cx_cursor));
}

/*
SourceRange Cursor::get_source_range() const {
  return SourceRange(clang_getCursorExtent(cx_cursor));
}
*/

std::string Cursor::get_spelling() const {
  return clang::ToString(clang_getCursorSpelling(cx_cursor));
}

std::string Cursor::get_display_name() const {
  return clang::ToString(clang_getCursorDisplayName(cx_cursor));
}

std::string Cursor::get_usr() const {
  return clang::ToString(clang_getCursorUSR(cx_cursor));
}

bool Cursor::is_definition() const {
  return clang_isCursorDefinition(cx_cursor);
}

Cursor Cursor::get_referenced() const {
  return Cursor(clang_getCursorReferenced(cx_cursor));
}

Cursor Cursor::get_canonical() const {
  return Cursor(clang_getCanonicalCursor(cx_cursor));
}

Cursor Cursor::get_definition() const {
  return Cursor(clang_getCursorDefinition(cx_cursor));
}

Cursor Cursor::get_semantic_parent() const {
  return Cursor(clang_getCursorSemanticParent(cx_cursor));
}

std::vector<Cursor> Cursor::get_arguments() const {
  int size = clang_Cursor_getNumArguments(cx_cursor);
  assert(size >= 0);
  if (size < 0)
    return std::vector<Cursor>();

  std::vector<Cursor> cursors(size);
  for (int i = 0; i < size; ++i)
    cursors.emplace_back(clang_Cursor_getArgument(cx_cursor, i));
  return cursors;
}

bool Cursor::is_valid_kind() const {
  CXCursor referenced = clang_getCursorReferenced(cx_cursor);
  if (clang_Cursor_isNull(referenced))
    return false;

  CXCursorKind kind = get_kind();
  return kind > CXCursor_UnexposedDecl &&
    (kind < CXCursor_FirstInvalid || kind > CXCursor_LastInvalid);
}

std::string Cursor::get_type_description() const {
  std::string spelling;

  auto referenced = clang_getCursorReferenced(cx_cursor);
  if (!clang_Cursor_isNull(referenced)) {
    auto type = clang_getCursorType(referenced);
    spelling = clang::ToString(clang_getTypeSpelling(type));

#if CINDEX_VERSION_MAJOR==0 && CINDEX_VERSION_MINOR<32
    const std::string auto_str = "auto";
    if (spelling.size() >= 4 && std::equal(auto_str.begin(), auto_str.end(), spelling.begin())) {
      auto canonical_type = clang_getCanonicalType(clang_getCursorType(cx_cursor));
      auto canonical_spelling = ToString(clang_getTypeSpelling(canonical_type));
      if (spelling.size() > 5 && spelling[4] == ' ' && spelling[5] == '&' && spelling != canonical_spelling)
        return canonical_spelling + " &";
      else
        return canonical_spelling;
    }

    const std::string const_auto_str = "const auto";
    if (spelling.size() >= 10 && std::equal(const_auto_str.begin(), const_auto_str.end(), spelling.begin())) {
      auto canonical_type = clang_getCanonicalType(clang_getCursorType(cx_cursor));
      auto canonical_spelling = ToString(clang_getTypeSpelling(canonical_type));
      if (spelling.size() > 11 && spelling[10] == ' ' && spelling[11] == '&' && spelling != canonical_spelling)
        return canonical_spelling + " &";
      else
        return canonical_spelling;
    }
#endif
  }

  if (spelling.empty())
    return get_spelling();

  return spelling;
}

std::string Cursor::get_comments() const {
  Cursor referenced = get_referenced();
  if (referenced)
    return clang::ToString(clang_Cursor_getRawCommentText(referenced.cx_cursor));

  return "";
}

std::string Cursor::ToString() const {
  return clang::ToString(get_kind()) + " " + get_spelling();
}

}  // namespace clang