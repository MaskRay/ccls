#ifndef COMPILECOMMANDS_H_
#define COMPILECOMMANDS_H_
#include "CompilationDatabase.h"
#include "CompileCommand.h"
#include <clang-c/CXCompilationDatabase.h>
#include <string>
#include <vector>

namespace clang {
  class CompileCommands {
  public:
    CompileCommands(const std::string &filename, CompilationDatabase &db);
    std::vector<CompileCommand> get_commands();
    ~CompileCommands();

    CXCompileCommands cx_commands;
  };
}
#endif  // COMPILECOMMANDS_H_
