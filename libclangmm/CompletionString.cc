#include "CompletionString.h"
#include "Utility.h"

clang::CompletionString::
CompletionString(const CXCompletionString &cx_completion_sting) : cx_completion_sting(cx_completion_sting) {}

bool clang::CompletionString::available() {
  return clang_getCompletionAvailability(cx_completion_sting) == CXAvailability_Available;
}

unsigned clang::CompletionString::get_num_chunks() {
    return clang_getNumCompletionChunks(cx_completion_sting);
}

std::vector<clang::CompletionChunk> clang::CompletionString::get_chunks() {
  std::vector<CompletionChunk> res;
  for (unsigned i = 0; i < get_num_chunks(); i++) {
    res.emplace_back(to_string(clang_getCompletionChunkText(cx_completion_sting, i)), static_cast<CompletionChunkKind> (clang_getCompletionChunkKind(cx_completion_sting, i)));
  }
  return res;
}

std::string clang::CompletionString::get_brief_comments() {
  return to_string(clang_getCompletionBriefComment(cx_completion_sting));
}

clang::CompletionChunk::CompletionChunk(std::string chunk, CompletionChunkKind kind) :
  chunk(chunk), kind(kind) { }
