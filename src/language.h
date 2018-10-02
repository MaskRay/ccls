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

#include "serializer.h"

#include <string_view>

// Used to identify the language at a file level. The ordering is important, as
// a file previously identified as `C`, will be changed to `Cpp` if it
// encounters a c++ declaration.
enum class LanguageId { Unknown = -1, C = 0, Cpp = 1, ObjC = 2, ObjCpp = 3 };
MAKE_REFLECT_TYPE_PROXY(LanguageId);

LanguageId SourceFileLanguage(std::string_view path);
const char *LanguageIdentifier(LanguageId lang);
