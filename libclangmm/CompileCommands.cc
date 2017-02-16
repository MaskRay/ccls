#include "CompileCommands.h"

clang::CompileCommands::CompileCommands(const std::string &filename, CompilationDatabase &db) {
  cx_commands =
    clang_CompilationDatabase_getCompileCommands(db.cx_db, filename.c_str());
  if(clang_CompileCommands_getSize(cx_commands)==0)
    cx_commands = clang_CompilationDatabase_getAllCompileCommands(db.cx_db);
}

clang::CompileCommands::~CompileCommands() {
  clang_CompileCommands_dispose(cx_commands);
}

std::vector<clang::CompileCommand> clang::CompileCommands::get_commands() {
  unsigned N = clang_CompileCommands_getSize(cx_commands);
  std::vector<CompileCommand> res;
  for (unsigned i = 0; i < N; i++) {
    res.emplace_back(clang_CompileCommands_getCommand(cx_commands, i));
  }
  return res;
}
