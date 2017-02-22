#pragma once

#include <clang-c/CXCompilationDatabase.h>
#include <vector>
#include <string>

namespace clang {
class CompileCommand {
public:
  CompileCommand(const CXCompileCommand& cx_command);
  std::string get_command() const;
  std::vector<std::string> get_command_as_args() const;

  CXCompileCommand cx_command;
};
}
