#include "CompilationDatabase.h"

#include <iostream>

clang::CompilationDatabase::CompilationDatabase(const std::string& project_path) {
  CXCompilationDatabase_Error error;
  cx_db = clang_CompilationDatabase_fromDirectory(project_path.c_str(), &error);
  if (error == CXCompilationDatabase_CanNotLoadDatabase) {
    std::cerr << "[FATAL]: Unable to load compile_commands.json located at \""
      << project_path << "\"";
    exit(1);
  }
}

clang::CompilationDatabase::~CompilationDatabase() {
  clang_CompilationDatabase_dispose(cx_db);
}
