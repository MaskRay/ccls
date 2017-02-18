#if false
#include "Tokens.h"
#include "Utility.h"

clang::Tokens::Tokens(CXTranslationUnit &cx_tu, const SourceRange &range): cx_tu(cx_tu) {
  clang_tokenize(cx_tu, range.cx_range, &cx_tokens, &num_tokens);
  cx_cursors.resize(num_tokens);
  clang_annotateTokens(cx_tu, cx_tokens, num_tokens, cx_cursors.data());
  for (unsigned i = 0; i < num_tokens; i++) {
    if(cx_cursors[i].kind==CXCursor_DeclRefExpr) { //Temporary fix to a libclang bug
      auto real_cursor=clang_getCursor(cx_tu, clang_getTokenLocation(cx_tu, cx_tokens[i]));
      cx_cursors[i]=real_cursor;
    }
    emplace_back(Token(cx_tu, cx_tokens[i], cx_cursors[i]));
  }
}

clang::Tokens::~Tokens() {
  clang_disposeTokens(cx_tu, cx_tokens, size());
}

//This works across TranslationUnits! However, to get rename refactoring to work, 
//one have to open all the files that might include a similar token
//Similar tokens defined as tokens with equal referenced cursors. 
std::vector<std::pair<clang::Offset, clang::Offset> > clang::Tokens::get_similar_token_offsets(CXCursorKind kind,
                                                                                               const std::string &spelling,
                                                                                               const std::string &usr) {
  std::vector<std::pair<Offset, Offset> > offsets;
  for(auto &token: *this) {
    if(token.is_identifier()) {
      auto referenced=token.get_cursor().get_referenced();
      if(referenced && kind==referenced.get_kind() && spelling==token.get_spelling() && usr==referenced.get_usr())
        offsets.emplace_back(token.offsets);
    }
  }
  return offsets;
}

#endif