#pragma once

#if USE_CLANG_CXX

#include "language_server_api.h"
#include "working_files.h"

#include <clang/Format/Format.h>

#include <vector>

std::vector<clang::tooling::Replacement> ClangFormatDocument(
    WorkingFile* working_file,
    int start,
    int end,
    lsFormattingOptions options);

#endif
