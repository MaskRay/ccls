#pragma once

#include <string>
#include <clang-c/CXCompilationDatabase.h>

namespace clang {

struct CompilationCommand {
  std::string path;
  std::string args;
};

class CompilationDatabase {
public:
  explicit CompilationDatabase(const std::string &project_path);
  ~CompilationDatabase();

  CXCompilationDatabase cx_db;
};
}
