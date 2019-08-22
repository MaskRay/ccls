/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#pragma once

#include <limits.h>
#include <string>
#include <string_view>

namespace ccls {
class FuzzyMatcher {
public:
  constexpr static int kMaxPat = 100;
  constexpr static int kMaxText = 200;
  // Negative but far from INT_MIN so that intermediate results are hard to
  // overflow.
  constexpr static int kMinScore = INT_MIN / 4;

  FuzzyMatcher(std::string_view pattern, int case_sensitivity);
  int match(std::string_view text, bool strict);

private:
  int case_sensitivity;
  std::string pat;
  std::string_view text;
  int pat_set, text_set;
  char low_pat[kMaxPat], low_text[kMaxText];
  int pat_role[kMaxPat], text_role[kMaxText];
  int dp[2][kMaxText + 1][2];

  int matchScore(int i, int j, bool last);
  int missScore(int j, bool last);
};
} // namespace ccls
