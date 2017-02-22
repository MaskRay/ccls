#include "CompileCommand.h"
#include "CompileCommands.h"
#include "Utility.h"

namespace clang {

CompileCommand::CompileCommand(const CXCompileCommand& command)
  : cx_command(command) {};

std::string CompileCommand::get_command() const {
  std::string result;
  unsigned int num_args = clang_CompileCommand_getNumArgs(cx_command);
  for (unsigned int i = 0; i < num_args; i++)
    result += ToString(clang_CompileCommand_getArg(cx_command, i));

  return result;
}

std::vector<std::string> CompileCommand::get_command_as_args() const {
  unsigned num_args = clang_CompileCommand_getNumArgs(cx_command);
  std::vector<std::string> result(num_args);
  for (unsigned i = 0; i < num_args; i++)
    result[i] = ToString(clang_CompileCommand_getArg(cx_command, i));

  return result;
}

}  // namespace clang