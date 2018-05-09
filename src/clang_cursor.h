#pragma once

#include "nt_string.h"
#include "position.h"

#include <clang-c/Index.h>
#include <optional>

#include <array>
#include <string>
#include <vector>

using Usr = uint64_t;

Range ResolveCXSourceRange(const CXSourceRange& range,
                           CXFile* cx_file = nullptr);

class ClangCursor;

class ClangType {
 public:
  ClangType() = default;
  ClangType(const CXType& cx) : cx_type(cx) {}

  // Returns true if this is a fundamental type like int.
  bool is_builtin() const {
    // NOTE: This will return false for pointed types. Should we call
    //       strip_qualifiers for the user?
    return cx_type.kind >= CXType_FirstBuiltin &&
           cx_type.kind <= CXType_LastBuiltin;
  }

  ClangCursor get_declaration() const;
  std::string get_usr() const;
  Usr get_usr_hash() const;
  std::string get_spell_name() const;
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
  ClangCursor() = default;
  ClangCursor(CXCursor cx) : cx_cursor(cx) {}
  bool operator==(const ClangCursor& o) const {
    return clang_equalCursors(cx_cursor, o.cx_cursor);
  }
  bool operator!=(const ClangCursor& o) const {
    return !(*this == o);
  }

  CXCursorKind get_kind() const {
    return cx_cursor.kind;
  }
  ClangType get_type() const;
  std::string get_spell_name() const;
  Range get_spell(CXFile* cx_file = nullptr) const;
  Range get_extent() const;
  std::string get_display_name() const;
  std::string get_usr() const;
  Usr get_usr_hash() const;
  std::optional<Usr> get_opt_usr_hash() const;

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
  ClangCursor get_lexical_parent() const;
  ClangCursor get_semantic_parent() const;
  std::vector<ClangCursor> get_arguments() const;
  bool is_valid_kind() const;

  std::string get_type_description() const;
  NtString get_comments() const;

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

// Simple RAII wrapper about CXIndex.
// Note: building a ClangIndex instance acquires a global lock, since libclang
// API does not appear to be thread-safe here.
class ClangIndex {
 public:
  ClangIndex();
  ClangIndex(int exclude_declarations_from_pch, int display_diagnostics);
  ~ClangIndex();
  CXIndex cx_index;
};
