#pragma once

#include "lsp.h"

#include <string_view.h>

#include <string>
#include <tuple>

// Utility method to map |position| to an offset inside of |content|.
int GetOffsetForPosition(lsPosition position, std::string_view content);
// Utility method to find a position for the given character.
lsPosition CharPos(std::string_view search,
                   char character,
                   int character_offset = 0);

// TODO: eliminate |line_number| param.
optional<lsRange> ExtractQuotedRange(int line_number, const std::string& line);

void LexFunctionDeclaration(const std::string& buffer_content,
                            lsPosition declaration_spelling,
                            optional<std::string> type_name,
                            std::string* insert_text,
                            int* newlines_after_name);

std::string_view LexIdentifierAroundPos(lsPosition position,
                                        std::string_view content);

std::pair<bool, int> CaseFoldingSubsequenceMatch(std::string_view search,
                                                 std::string_view content);
