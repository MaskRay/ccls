#pragma once

#include <optional.h>

#include <clang-c/Index.h>

#include <string>
#include <vector>

using std::experimental::optional;

class ClangType {
 public:
  ClangType();
  ClangType(const CXType& other);

  bool operator==(const ClangType& rhs) const;

  // Returns true if this is a fundamental type like int.
  bool is_fundamental() const;

  // ClangCursor is not defined so we have to return CXCursor
  CXCursor get_declaration() const;
  std::string get_usr() const;
  std::string get_spelling() const;
  ClangType get_canonical() const;

  // Try to resolve this type and remove qualifies, ie, Foo* will become Foo
  ClangType strip_qualifiers() const;

  ClangType get_return_type() const;
  std::vector<ClangType> get_arguments() const;
  std::vector<ClangType> get_template_arguments() const;

  CXType cx_type;
};

class ClangCursor {
 public:
  ClangCursor();
  ClangCursor(const CXCursor& other);

  explicit operator bool() const;
  bool operator==(const ClangCursor& rhs) const;
  bool operator!=(const ClangCursor& rhs) const;

  CXCursorKind get_kind() const;
  ClangCursor get_declaration() const;
  ClangType get_type() const;
  std::string get_spelling() const;
  std::string get_display_name() const;
  std::string get_usr() const;

  bool is_definition() const;

  // If the given cursor points to a template specialization, this
  // will return the cursor pointing to the template definition.
  // If the given cursor is not a template specialization, this will
  // just return the same cursor.
  //
  // This means it is always safe to call this method.
  ClangCursor template_specialization_to_template_definition() const;

  ClangCursor get_referenced() const;
  ClangCursor get_canonical() const;
  ClangCursor get_definition() const;
  ClangCursor get_semantic_parent() const;
  std::vector<ClangCursor> get_arguments() const;
  bool is_valid_kind() const;

  std::string get_type_description() const;
  optional<std::string> get_comments() const;

  std::string ToString() const;

  enum class VisitResult { Break, Continue, Recurse };

  template <typename TClientData>
  using Visitor = VisitResult (*)(ClangCursor cursor,
                                  ClangCursor parent,
                                  TClientData* client_data);

  template <typename TClientData>
  void VisitChildren(Visitor<TClientData> visitor,
                     TClientData* client_data) const {
    clang_visitChildren(cx_cursor, reinterpret_cast<CXCursorVisitor>(visitor),
                        client_data);
  }

  CXCursor cx_cursor;
};

namespace std {
template <>
struct hash<ClangCursor> {
  size_t operator()(const ClangCursor& x) const {
    return clang_hashCursor(x.cx_cursor);
  }
};
}  // namespace std
