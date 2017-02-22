#pragma once

#include <string>
#include <vector>
#include <clang-c/CXCompilationDatabase.h>

#include "CompilationDatabase.h"
#include "CompileCommand.h"

namespace clang {
class CompileCommands {
public:
  CompileCommands(const CompilationDatabase& db);
  std::vector<CompileCommand> get_commands();
  ~CompileCommands();

  CXCompileCommands cx_commands;
};
}
