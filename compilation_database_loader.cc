#include "compilation_database_loader.h"

#include <iostream>
#include <clang-c/CXCompilationDatabase.h>

#include "libclangmm/Utility.h"

#include "utils.h"

// See http://stackoverflow.com/a/2072890
bool EndsWith(const std::string& value, const std::string& ending) {
  if (ending.size() > value.size())
    return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

bool StartsWith(const std::string& value, const std::string& start) {
  return std::equal(start.begin(), start.end(), value.begin());
}

std::vector<CompilationEntry> LoadFromDirectoryListing(const std::string& project_directory) {
  std::vector<CompilationEntry> result;

  std::vector<std::string> args;
  for (const std::string& line : ReadLines(project_directory + "/clang_args")) {
    if (line.empty() || StartsWith(line, "#"))
      continue;
    std::cerr << "Adding argument " << line << std::endl;
    args.push_back(line);
  }


  std::vector<std::string> files = GetFilesInFolder(project_directory, true /*recursive*/, false /*add_folder_to_path*/);
  for (const std::string& file : files) {
    if (EndsWith(file, ".cc") || EndsWith(file, ".cpp") ||
      EndsWith(file, ".c") || EndsWith(file, ".h") ||
      EndsWith(file, ".hpp")) {

      CompilationEntry entry;
      entry.directory = ".";
      entry.filename = file;
      entry.args = {};
      result.push_back(entry);
    }
  }

  return result;
}

std::vector<CompilationEntry> LoadCompilationEntriesFromDirectory(const std::string& project_directory) {
  CXCompilationDatabase_Error cx_db_load_error;
  CXCompilationDatabase cx_db = clang_CompilationDatabase_fromDirectory(project_directory.c_str(), &cx_db_load_error);
  if (cx_db_load_error == CXCompilationDatabase_CanNotLoadDatabase) {
    std::cerr << "Unable to load compile_commands.json located at \"" << project_directory << "\"; using directory listing instead." << std::endl;
    return LoadFromDirectoryListing(project_directory);
  }

  CXCompileCommands cx_commands = clang_CompilationDatabase_getAllCompileCommands(cx_db);

  unsigned int num_commands = clang_CompileCommands_getSize(cx_commands);
  std::vector<CompilationEntry> result;
  for (unsigned int i = 0; i < num_commands; i++) {
    CXCompileCommand cx_command = clang_CompileCommands_getCommand(cx_commands, i);
    CompilationEntry entry;

    entry.directory = clang::ToString(clang_CompileCommand_getDirectory(cx_command));
    entry.filename = clang::ToString(clang_CompileCommand_getFilename(cx_command));

    unsigned int num_args = clang_CompileCommand_getNumArgs(cx_command);
    entry.args.reserve(num_args);
    for (unsigned int j = 0; j < num_args; ++j) {
      entry.args.push_back(clang::ToString(clang_CompileCommand_getArg(cx_command, j)));
    }

    result.push_back(entry);
  }

  clang_CompileCommands_dispose(cx_commands);
  clang_CompilationDatabase_dispose(cx_db);

  return result;
}