#ifndef COMPILECOMMAND_H_
#define COMPILECOMMAND_H_
#include <clang-c/CXCompilationDatabase.h>
#include <vector>
#include <string>

namespace clang {
  class CompileCommand {
  public:
    CompileCommand(const CXCompileCommand& cx_command) : cx_command(cx_command) {};
    std::string get_command();
    std::vector<std::string> get_command_as_args();

    CXCompileCommand cx_command;
  };
}
#endif  // COMPILECOMMAND_H_
