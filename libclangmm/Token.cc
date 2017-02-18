#if false
#include "Token.h"
#include "Utility.h"

// // // // //
//  Token   //
// // // // //

// returns gets an source location for this token objekt
// based on the translationunit given
clang::SourceLocation clang::Token::get_source_location() const {
  return SourceLocation(clang_getTokenLocation(cx_tu, cx_token));
}

// returns a sourcerange that covers this token
clang::SourceRange clang::Token::get_source_range() const {
  return SourceRange(clang_getTokenExtent(cx_tu, cx_token));
}
// returns a string description of this tokens kind
std::string clang::Token::get_spelling() const {
  return ToString(clang_getTokenSpelling(cx_tu, cx_token));
}

clang::Token::Kind clang::Token::get_kind() const {
  return static_cast<Kind>(clang_getTokenKind(cx_token));
}

bool clang::Token::is_identifier() const {
  auto token_kind=get_kind();
  auto cursor=get_cursor();
  if(token_kind==clang::Token::Kind::Identifier && cursor.is_valid_kind())
    return true;
  else if(token_kind==clang::Token::Kind::Keyword && cursor.is_valid_kind()) {
    auto spelling=get_spelling();
    if(spelling=="operator" || (spelling=="bool" && get_cursor().get_spelling()=="operator bool"))
      return true;
  }
  else if(token_kind==clang::Token::Kind::Punctuation && cursor.is_valid_kind()) {
    auto referenced=get_cursor().get_referenced();
    if(referenced) {
      auto referenced_kind=referenced.get_kind();
      if(referenced_kind== CXCursor_FunctionDecl || referenced_kind==CXCursor_CXXMethod || referenced_kind==CXCursor_Constructor)
        return true;
    }
  }
  return false;
}

#endif