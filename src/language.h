// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "serializer.h"

#include <string_view>

// Used to identify the language at a file level. The ordering is important, as
// a file previously identified as `C`, will be changed to `Cpp` if it
// encounters a c++ declaration.
enum class LanguageId { Unknown = -1, C = 0, Cpp = 1, ObjC = 2, ObjCpp = 3 };
MAKE_REFLECT_TYPE_PROXY(LanguageId);

LanguageId SourceFileLanguage(std::string_view path);
const char *LanguageIdentifier(LanguageId lang);
