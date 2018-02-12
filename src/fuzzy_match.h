#pragma once

#include <string_view.h>

#include <limits.h>
#include <vector>

// Negative but far from INT_MIN so that intermediate results are hard to
// overflow
constexpr int kMinScore = INT_MIN / 2;

// Evaluate the score matching |pattern| against |str|, the larger the better.
// |score| and |dp| must be at least as long as |str|.
int FuzzyEvaluate(std::string_view pattern,
                  std::string_view str,
                  std::vector<int>& score,
                  std::vector<int>& dp);
