#pragma once

#include "lsp.h"

#include <string_view>
#include <tuple>

// Utility method to map |position| to an offset inside of |content|.
int GetOffsetForPosition(lsPosition position, std::string_view content);

std::string_view LexIdentifierAroundPos(lsPosition position,
                                        std::string_view content);

std::pair<bool, int> CaseFoldingSubsequenceMatch(std::string_view search,
                                                 std::string_view content);
