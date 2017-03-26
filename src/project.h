#pragma once

#include "compilation_database_loader.h" // TODO: merge compilation_database_loader into this file.

#include <optional.h>

using std::experimental::optional;
using std::experimental::nullopt;

struct Project {
  std::vector<CompilationEntry> entries;

  optional<CompilationEntry> FindCompilationEntryForFile(const std::string& filename);
};