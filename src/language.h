#pragma once

#include "serializer.h"

// Used to identify the language at a file level. The ordering is important, as
// a file previously identified as `C`, will be changed to `Cpp` if it
// encounters a c++ declaration.
enum class LanguageId { Unknown = 0, C = 1, Cpp = 2, ObjC = 3, ObjCpp = 4 };
MAKE_REFLECT_TYPE_PROXY(LanguageId);
