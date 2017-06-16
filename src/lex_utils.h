#pragma once

#include "language_server_api.h"

#include <string>


// Utility method to map |position| to an offset inside of |content|.
int GetOffsetForPosition(lsPosition position, const std::string& content);
// Utility method to find a position for the given character.
lsPosition CharPos(const std::string& search, char character, int character_offset = 0);

bool ShouldRunIncludeCompletion(const std::string& line);

// TODO: eliminate |line_number| param.
optional<lsRange> ExtractQuotedRange(int line_number, const std::string& line);

void LexFunctionDeclaration(const std::string& buffer_content, lsPosition declaration_spelling, optional<std::string> type_name, std::string* insert_text, int* newlines_after_name);

std::string LexWordAroundPos(lsPosition position, const std::string& content);

bool SubstringMatch(const std::string& search, const std::string& content);