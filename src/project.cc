#include "project.h"

#include "libclangmm/Utility.h"
#include "platform.h"
#include "serializer.h"
#include "utils.h"

#include <clang-c/CXCompilationDatabase.h>

#include <iostream>
#include <vector>


struct CompileCommandsEntry {
  std::string directory;
  std::string file;
  std::string command;
  std::vector<std::string> args;
};
MAKE_REFLECT_STRUCT(CompileCommandsEntry, directory, file, command, args);

namespace {



// https://github.com/Andersbakken/rtags/blob/6b16b81ea93aeff4a58930b44b2a0a207b456192/src/Source.cpp
static const char *kValueArgs[] = {
  "--param",
  "-G",
  "-MF",
  "-MQ",
  "-MT",
  "-T",
  "-V",
  "-Xanalyzer",
  "-Xassembler",
  "-Xclang",
  "-Xlinker",
  "-Xpreprocessor",
  "-arch",
  "-b",
  "-gcc-toolchain",
  //"-imacros",
  "-imultilib",
  //"-include",
  //"-iprefix",
  //"-isysroot",
  "-ivfsoverlay",
  "-iwithprefix",
  "-iwithprefixbefore",
  "-o",
  "-target",
  "-x"
};
static const char *kBlacklist[] = {
  "--param",
  "-M",
  "-MD",
  "-MF",
  "-MG",
  "-MM",
  "-MMD",
  "-MP",
  "-MQ",
  "-MT",
  "-Og",
  "-Wa,--32",
  "-Wa,--64",
  "-Wl,--incremental-full",
  "-Wl,--incremental-patch,1",
  "-Wl,--no-incremental",
  "-fbuild-session-file=",
  "-fbuild-session-timestamp=",
  "-fembed-bitcode",
  "-fembed-bitcode-marker",
  "-fmodules-validate-once-per-build-session",
  "-fno-delete-null-pointer-checks",
  "-fno-use-linker-plugin"
  "-fno-var-tracking",
  "-fno-var-tracking-assignments",
  "-fno-enforce-eh-specs",
  "-fvar-tracking",
  "-fvar-tracking-assignments",
  "-fvar-tracking-assignments-toggle",
  "-gcc-toolchain",
  "-march=",
  "-masm=",
  "-mcpu=",
  "-mfpmath=",
  "-mtune=",
  "-s",

  //"-B",
  //"-f",
  //"-pipe",
  //"-W",
  // TODO
  "-Wno-unused-lambda-capture",
  "/",
  "..",
};

Project::Entry GetCompilationEntryFromCompileCommandEntry(const CompileCommandsEntry& entry) {
  Project::Entry result;
  result.filename = NormalizePath(entry.file);

  size_t num_args = entry.args.size();
  result.args.reserve(num_args);
  for (size_t j = 0; j < num_args; ++j) {
    std::string arg = entry.args[j];


    bool bad = false;
    for (auto& entry : kValueArgs) {
      if (StartsWith(arg, entry)) {
        bad = true;
        continue;
      }
    }
    if (bad) {
      ++j;
      continue;
    }


    for (auto& entry : kBlacklist) {
      if (StartsWith(arg, entry)) {
        bad = true;
        continue;
      }
    }
    if (bad) {
      continue;
    }


    if (StartsWith(arg, "-I")) {
      std::string path = entry.directory + "/" + arg.substr(2);
      path = NormalizePath(path);
      arg = "-I" + path;
    }

    result.args.push_back(arg);
    //if (StartsWith(arg, "-I") || StartsWith(arg, "-D") || StartsWith(arg, "-std"))
  }

  // TODO/fixme
  result.args.push_back("-xc++");
  result.args.push_back("-std=c++11");

  return result;
}

std::vector<Project::Entry> LoadFromCompileCommandsJson(const std::string& project_directory) {
  optional<std::string> compile_commands_content = ReadContent(project_directory + "/compile_commands.json");
  if (!compile_commands_content)
    return {};

  rapidjson::Document reader;
  reader.Parse(compile_commands_content->c_str());
  if (reader.HasParseError())
    return {};

  std::vector<CompileCommandsEntry> entries;
  Reflect(reader, entries);

  std::vector<Project::Entry> result;
  result.reserve(entries.size());
  for (const auto& entry : entries)
    result.push_back(GetCompilationEntryFromCompileCommandEntry(entry));
  return result;
}

std::vector<Project::Entry> LoadFromDirectoryListing(const std::string& project_directory) {
  std::vector<Project::Entry> result;

  std::vector<std::string> args;
  std::cerr << "Using arguments: ";
  for (const std::string& line : ReadLines(project_directory + "/clang_args")) {
    if (line.empty() || StartsWith(line, "#"))
      continue;
    if (!args.empty())
      std::cerr << ", ";
    std::cerr << line;
    args.push_back(line);
  }
  std::cerr << std::endl;


  std::vector<std::string> files = GetFilesInFolder(project_directory, true /*recursive*/, true /*add_folder_to_path*/);
  for (const std::string& file : files) {
    if (EndsWith(file, ".cc") || EndsWith(file, ".cpp") || EndsWith(file, ".c")) {
      Project::Entry entry;
      entry.filename = NormalizePath(file);
      entry.args = args;
      result.push_back(entry);
    }
  }

  return result;
}

std::vector<Project::Entry> LoadCompilationEntriesFromDirectory(const std::string& project_directory) {
  // TODO: Figure out if this function or the clang one is faster.
  //return LoadFromCompileCommandsJson(project_directory);

  CXCompilationDatabase_Error cx_db_load_error;
  CXCompilationDatabase cx_db = clang_CompilationDatabase_fromDirectory(project_directory.c_str(), &cx_db_load_error);
  if (cx_db_load_error == CXCompilationDatabase_CanNotLoadDatabase) {
    std::cerr << "Unable to load compile_commands.json located at \"" << project_directory << "\"; using directory listing instead." << std::endl;
    return LoadFromDirectoryListing(project_directory);
  }

  CXCompileCommands cx_commands = clang_CompilationDatabase_getAllCompileCommands(cx_db);

  unsigned int num_commands = clang_CompileCommands_getSize(cx_commands);
  std::vector<Project::Entry> result;
  for (unsigned int i = 0; i < num_commands; i++) {
    CXCompileCommand cx_command = clang_CompileCommands_getCommand(cx_commands, i);

    std::string directory = clang::ToString(clang_CompileCommand_getDirectory(cx_command));
    std::string relative_filename = clang::ToString(clang_CompileCommand_getFilename(cx_command));
    std::string absolute_filename = directory + "/" + relative_filename;

    CompileCommandsEntry entry;
    entry.file = NormalizePath(absolute_filename);
    entry.directory = directory;

    unsigned num_args = clang_CompileCommand_getNumArgs(cx_command);
    entry.args.reserve(num_args);
    for (unsigned i = 0; i < num_args; ++i)
      entry.args.push_back(clang::ToString(clang_CompileCommand_getArg(cx_command, i)));

    result.push_back(GetCompilationEntryFromCompileCommandEntry(entry));
  }

  clang_CompileCommands_dispose(cx_commands);
  clang_CompilationDatabase_dispose(cx_db);

  return result;
}
}  // namespace

void Project::Load(const std::string& directory) {
  entries = LoadCompilationEntriesFromDirectory(directory);

  absolute_path_to_entry_index_.resize(entries.size());
  for (int i = 0; i < entries.size(); ++i)
    absolute_path_to_entry_index_[entries[i].filename] = i;
}

optional<Project::Entry> Project::FindCompilationEntryForFile(const std::string& filename) {
  // TODO: There might be a lot of thread contention here.
  std::lock_guard<std::mutex> lock(entries_modification_mutex_);

  auto it = absolute_path_to_entry_index_.find(filename);
  if (it != absolute_path_to_entry_index_.end())
    return entries[it->second];
  return nullopt;
}

void Project::UpdateFileState(const std::string& filename, const std::string& import_file, const std::vector<std::string>& import_dependencies, uint64_t modification_time) {
  {
    // TODO: There might be a lot of thread contention here.
    std::lock_guard<std::mutex> lock(entries_modification_mutex_);
    auto it = absolute_path_to_entry_index_.find(filename);
    if (it != absolute_path_to_entry_index_.end()) {
      auto& entry = entries[it->second];
      entry.import_file = import_file;
      entry.import_dependencies = import_dependencies;
      entry.last_modification_time = modification_time;
      return;
    }
  }

  {
    optional<Project::Entry> import_entry = FindCompilationEntryForFile(import_file);

    Project::Entry entry;
    entry.filename = filename;
    if (import_entry) {
      entry.args = import_entry->args;
    }

    entry.import_file = import_file;
    entry.import_dependencies = import_dependencies;
    entry.last_modification_time = modification_time;
    
    // TODO: There might be a lot of thread contention here.
    std::lock_guard<std::mutex> lock(entries_modification_mutex_);
    absolute_path_to_entry_index_[filename] = entries.size();
    entries.push_back(entry);
  }
}
