#pragma once

#include "indexer.h"

std::tuple<std::string, int16_t, int16_t, int16_t> GetFunctionSignature(
    IndexFile* db,
    NamespaceHelper* ns,
    const CXIdxDeclInfo* decl);
