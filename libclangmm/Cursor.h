#ifndef CURSOR_H_
#define CURSOR_H_

#include <string>
#include <vector>
#include <type_traits>

#include <clang-c/Index.h>
#include "SourceLocation.h"
#include "SourceRange.h"


namespace clang {

class Type {
public:
  Type();
  Type(const CXType& other);

  bool operator==(const Type& rhs) const;

  // Returns true if this is a fundamental type like int.
  bool is_fundamental() const;

  std::string get_usr() const;
  std::string get_spelling() const;

  // Try to resolve this type and remove qualifies, ie, Foo* will become Foo
  Type strip_qualifiers() const;

  Type get_return_type() const;
  std::vector<Type> get_arguments() const;
  std::vector<Type> get_template_arguments() const;

  CXType cx_type;
};

enum class VisiterResult {
  Break,
  Continue,
  Recurse
};

class Cursor {
public:
  Cursor();
  Cursor(const CXCursor& other);

  operator bool() const;
  bool operator==(const Cursor& rhs) const;

  CXCursorKind get_kind() const;
  Type get_type() const;
  SourceLocation get_source_location() const;
  //SourceRange get_source_range() const;
  std::string get_spelling() const;
  std::string get_display_name() const;
  std::string get_usr() const;

  bool is_definition() const;

  Cursor get_referenced() const;
  Cursor get_canonical() const;
  Cursor get_definition() const;
  Cursor get_semantic_parent() const;
  std::vector<Cursor> get_arguments() const;
  bool is_valid_kind() const;

  std::string evaluate() const;

  std::string get_type_description() const;
  std::string get_comments() const;

  std::string ToString() const;

  template<typename TClientData>
  using Visitor = VisiterResult(*)(Cursor cursor, Cursor parent, TClientData* client_data);

  enum class VisitResult {
    Completed, EndedEarly
  };

  template<typename TClientData>
  VisitResult VisitChildren(Visitor<TClientData> visitor, TClientData* client_data) const {
    if (clang_visitChildren(cx_cursor, reinterpret_cast<CXCursorVisitor>(visitor), client_data) == 0)
      return VisitResult::Completed;
    return VisitResult::EndedEarly;
  }

  CXCursor cx_cursor;
};
}  // namespace clang

#endif  // CURSOR_H_
