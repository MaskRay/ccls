#include "Cursor.h"
#include "Utility.h"
#include <algorithm>

namespace clang {

static_assert(sizeof(Cursor) == sizeof(CXCursor), "Cursor must be the same size as CXCursor");

std::string Type::get_spelling() const {
  return ToString(clang_getTypeSpelling(cx_type));
}

Type Cursor::get_type() const {
  return Type(clang_getCursorType(cx_cursor));
}

bool Type::operator==(const Type& rhs) const {
  return clang_equalTypes(cx_type, rhs.cx_type);
}

Type Type::get_result() const {
  return Type(clang_getResultType(cx_type));
}

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

SourceLocation Cursor::get_source_location() const {
  return SourceLocation(clang_getCursorLocation(cx_cursor));
}

SourceRange Cursor::get_source_range() const {
  return SourceRange(clang_getCursorExtent(cx_cursor));
}

std::string Cursor::get_spelling() const {
  return ToString(clang_getCursorSpelling(cx_cursor));
}

std::string Cursor::get_display_name() const {
  return ToString(clang_getCursorDisplayName(cx_cursor));
}

std::string Cursor::get_usr() const {
  return ToString(clang_getCursorUSR(cx_cursor));
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
  auto size = clang_Cursor_getNumArguments(cx_cursor);
  std::vector<Cursor> cursors(size);
  for (int c = 0; c < size; ++c)
    cursors.emplace_back(clang_Cursor_getArgument(cx_cursor, c));
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
    spelling = ToString(clang_getTypeSpelling(type));

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
    return ToString(clang_Cursor_getRawCommentText(referenced.cx_cursor));

  return "";
}

}  // namespace clang