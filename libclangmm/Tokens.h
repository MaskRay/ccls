#if false
#ifndef TOKENS_H_
#define TOKENS_H_
#include <clang-c/Index.h>
#include "SourceRange.h"
#include "Token.h"
#include <unordered_map>
#include <vector>

namespace clang {
  class Tokens : public std::vector<clang::Token> {
    friend class TranslationUnit;
    friend class Diagnostic;
    Tokens(CXTranslationUnit &cx_tu, const SourceRange &range);
  public:
    ~Tokens();
    std::vector<std::pair<clang::Offset, clang::Offset> > get_similar_token_offsets(CXCursorKind kind,
                                                                                    const std::string &spelling,
                                                                                    const std::string &usr);
  private:
    CXToken *cx_tokens;
    unsigned num_tokens;
    std::vector<CXCursor> cx_cursors;
    CXTranslationUnit& cx_tu;
  };
}  // namespace clang
#endif  // TOKENS_H_

#endif