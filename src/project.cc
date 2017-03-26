#include "project.h"

optional<CompilationEntry> Project::FindCompilationEntryForFile(const std::string& filename) {
  for (auto& entry : entries) {
    if (filename == entry.filename)
      return entry;
  }

  return nullopt;
}
