#ifndef CURSOR_H_
#define CURSOR_H_

#include <string>
#include <vector>
#include <clang-c/Index.h>

#include "SourceLocation.h"
#include "SourceRange.h"


namespace clang {
  class Cursor {
  public:

    class Type {
    public:
      Type(const CXType &cx_type) : cx_type(cx_type) {}
      std::string get_spelling() const;
      Type get_result() const;
      bool operator==(const Cursor::Type& rhs) const;
      
      CXType cx_type;
    };
    
    Cursor();
    explicit Cursor(const CXCursor& cx_cursor);

    CXCursorKind get_kind() const;
    Type get_type() const;
    SourceLocation get_source_location() const;
    SourceRange get_source_range() const;
    std::string get_spelling() const;
    std::string get_display_name() const;
    std::string get_usr() const;
    Cursor get_referenced() const;
    Cursor get_canonical() const;
    Cursor get_definition() const;
    Cursor get_semantic_parent() const;
    std::vector<Cursor> get_arguments() const;
    operator bool() const;
    bool operator==(const Cursor& rhs) const;
    
    bool is_valid_kind() const;
    std::string get_type_description() const;
    std::string get_brief_comments() const;
    
    CXCursor cx_cursor = clang_getNullCursor();
  };
}  // namespace clang
#endif  // CURSOR_H_
