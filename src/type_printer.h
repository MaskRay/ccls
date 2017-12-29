#pragma once

#include "indexer.h"

std::string GetFunctionSignature(IndexFile* db,
                                 NamespaceHelper* ns,
                                 const CXIdxDeclInfo* decl);
