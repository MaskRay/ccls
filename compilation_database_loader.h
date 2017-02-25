#pragma once

#include <string>
#include <vector>

struct CompilationEntry {
  std::string directory;
  std::string filename;
  std::vector<std::string> args;
};

// TODO: Add support for loading when there is no compilation_database.json
//       file. We will just recursively scan the directory and support a global
//       set of defines and include directories.

std::vector<CompilationEntry> LoadCompilationEntriesFromDirectory(const std::string& project_directory);