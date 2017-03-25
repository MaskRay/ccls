#ifndef COMPLETIONSTRING_H_
#define COMPLETIONSTRING_H_
#include <clang-c/Index.h>
#include <string>
#include <vector>

namespace clang {
  enum CompletionChunkKind {
    CompletionChunk_Optional,  CompletionChunk_TypedText,
    CompletionChunk_Text, CompletionChunk_Placeholder,
    CompletionChunk_Informative, CompletionChunk_CurrentParameter,
    CompletionChunk_LeftParen, CompletionChunk_RightParen,
    CompletionChunk_LeftBracket, CompletionChunk_RightBracket,
    CompletionChunk_LeftBrace, CompletionChunk_RightBrace,
    CompletionChunk_LeftAngle, CompletionChunk_RightAngle,
    CompletionChunk_Comma, CompletionChunk_ResultType,
    CompletionChunk_Colon, CompletionChunk_SemiColon,
    CompletionChunk_Equal, CompletionChunk_HorizontalSpace,
    CompletionChunk_VerticalSpace
  };

  class CompletionChunk {
  public:
    CompletionChunk(std::string chunk, CompletionChunkKind kind);
    std::string chunk;
    CompletionChunkKind kind;
  };

  class CompletionString {
  public:
    explicit CompletionString(const CXCompletionString &cx_completion_sting);
    bool available();
    std::vector<CompletionChunk> get_chunks();
    std::string get_brief_comments();
    unsigned get_num_chunks();
    
    CXCompletionString cx_completion_sting;
  };
}  // namespace clang
#endif  // COMPLETIONSTRING_H_
