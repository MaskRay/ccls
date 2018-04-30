#pragma once

#include "lsp.h"

#include <string_view>
#include <tuple>

// Utility method to map |position| to an offset inside of |content|.
int GetOffsetForPosition(lsPosition position, std::string_view content);

std::string_view LexIdentifierAroundPos(lsPosition position,
                                        std::string_view content);

int ReverseSubseqMatch(std::string_view pat,
                       std::string_view text,
                       int case_sensitivity);
