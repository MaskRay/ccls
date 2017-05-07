#include "project.h"

#include "fuzzy.h"
#include "libclangmm/Utility.h"
#include "platform.h"
#include "serializer.h"
#include "utils.h"

#include <clang-c/CXCompilationDatabase.h>
#include <doctest/doctest.h>

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

// Blacklisted flags which are always removed from the command line.
static const char *kBlacklist[] = {
  "-stdlib=libc++"
};

// Arguments which are followed by a potentially relative path. We need to make
// all relative paths absolute, otherwise libclang will not resolve them.
const char* kPathArgs[] = {
  "-isystem",
  "-I",
  "-iquote",
  "--sysroot="
};

Project::Entry GetCompilationEntryFromCompileCommandEntry(const std::vector<std::string>& extra_flags, const CompileCommandsEntry& entry) {
  Project::Entry result;
  result.filename = NormalizePath(entry.file);

  bool make_next_flag_absolute = false;

  result.args.reserve(entry.args.size() + extra_flags.size());
  for (size_t i = 0; i < entry.args.size(); ++i) {
    std::string arg = entry.args[i];

    // If blacklist skip.
    if (std::any_of(std::begin(kBlacklist), std::end(kBlacklist), [&arg](const char* value) {
      return StartsWith(arg, value);
    })) {
      continue;
    }

    // Cleanup path for previous argument.
    if (make_next_flag_absolute) {
      if (arg.size() > 0 && arg[0] != '/')
        arg = NormalizePath(entry.directory + arg);
      make_next_flag_absolute = false;
    }

    // Update arg if it is a path.
    for (const char* flag_type : kPathArgs) {
      if (arg == flag_type) {
        make_next_flag_absolute = true;
        break;
      }

      if (StartsWith(arg, flag_type)) {
        std::string path = arg.substr(strlen(flag_type));
        if (path.size() > 0 && path[0] != '/') {
          path = NormalizePath(entry.directory + "/" + path);
          arg = flag_type + path;
        }
        break;
      }
    }
    if (make_next_flag_absolute)
      continue;

    result.args.push_back(arg);
  }

  // We don't do any special processing on user-given extra flags.
  for (const auto& flag : extra_flags)
    result.args.push_back(flag);

  // Clang does not have good hueristics for determining source language. We
  // default to C++11 if the user has not specified.
  if (!StartsWithAny(entry.args, "-x"))
    result.args.push_back("-xc++");
  if (!StartsWithAny(entry.args, "-std="))
    result.args.push_back("-std=c++11");

  return result;
}

std::vector<Project::Entry> LoadFromCompileCommandsJson(const std::vector<std::string>& extra_flags, const std::string& project_directory) {
  // TODO: Fix this function, it may be way faster than libclang's implementation.

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
    result.push_back(GetCompilationEntryFromCompileCommandEntry(extra_flags, entry));
  return result;
}

std::vector<Project::Entry> LoadFromDirectoryListing(const std::vector<std::string>& extra_flags, const std::string& project_directory) {
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
  for (const std::string& flag : extra_flags) {
    std::cerr << flag << std::endl;
    args.push_back(flag);
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

std::vector<Project::Entry> LoadCompilationEntriesFromDirectory(const std::vector<std::string>& extra_flags, const std::string& project_directory) {
  // TODO: Figure out if this function or the clang one is faster.
  //return LoadFromCompileCommandsJson(project_directory);

  std::cerr << "Trying to load compile_commands.json" << std::endl;
  CXCompilationDatabase_Error cx_db_load_error;
  CXCompilationDatabase cx_db = clang_CompilationDatabase_fromDirectory(project_directory.c_str(), &cx_db_load_error);
  if (cx_db_load_error == CXCompilationDatabase_CanNotLoadDatabase) {
    std::cerr << "Unable to load compile_commands.json located at \"" << project_directory << "\"; using directory listing instead." << std::endl;
    return LoadFromDirectoryListing(extra_flags, project_directory);
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

    result.push_back(GetCompilationEntryFromCompileCommandEntry(extra_flags, entry));
  }

  clang_CompileCommands_dispose(cx_commands);
  clang_CompilationDatabase_dispose(cx_db);

  return result;
}

// Computes a score based on how well |a| and |b| match. This is used for
// argument guessing.
int ComputeGuessScore(const std::string& a, const std::string& b) {
  int score = 0;
  int i = 0;

  for (i = 0; i < a.length() && i < b.length(); ++i) {
    if (a[i] != b[i])
      break;
    ++score;
  }

  for (int j = i; j < a.length(); ++j) {
    if (a[j] == '/')
      --score;
  }
  for (int j = i; j < b.length(); ++j) {
    if (b[j] == '/')
      --score;
  }

  return score;
}

}  // namespace

void Project::Load(const std::vector<std::string>& extra_flags, const std::string& directory) {
  entries = LoadCompilationEntriesFromDirectory(extra_flags, directory);

  absolute_path_to_entry_index_.resize(entries.size());
  for (int i = 0; i < entries.size(); ++i)
    absolute_path_to_entry_index_[entries[i].filename] = i;
}

Project::Entry Project::FindCompilationEntryForFile(const std::string& filename) {
  auto it = absolute_path_to_entry_index_.find(filename);
  if (it != absolute_path_to_entry_index_.end())
    return entries[it->second];

  // We couldn't find the file. Try to infer it.
  // TODO: Cache inferred file in a separate array (using a lock or similar)
  Entry* best_entry = nullptr;
  int best_score = 0;
  for (Entry& entry : entries) {
    int score = ComputeGuessScore(filename, entry.filename);
    if (score > best_score) {
      best_score = score;
      best_entry = &entry;
    }
  }

  Project::Entry result;
  result.is_inferred = true;
  result.filename = filename;
  if (best_entry)
    result.args = best_entry->args;
  return result;
}

void Project::ForAllFilteredFiles(IndexerConfig* config, std::function<void(int i, const Entry& entry)> action) {
  std::vector<Matcher> whitelist;
  std::cerr << "Using whitelist" << std::endl;
  for (const std::string& entry : config->whitelist) {
    std::cerr << " - " << entry << std::endl;
    whitelist.push_back(Matcher(entry));
  }

  std::vector<Matcher> blacklist;
  std::cerr << "Using blacklist" << std::endl;
  for (const std::string& entry : config->blacklist) {
    std::cerr << " - " << entry << std::endl;
    blacklist.push_back(Matcher(entry));
  }


  for (int i = 0; i < entries.size(); ++i) {
    const Project::Entry& entry = entries[i];
    std::string filepath = entry.filename;

    const Matcher* is_bad = nullptr;
    for (const Matcher& m : whitelist) {
      if (!m.IsMatch(filepath)) {
        is_bad = &m;
        break;
      }
    }
    if (is_bad) {
      std::cerr << "[" << i << "/" << (entries.size() - 1) << "] Failed whitelist check \"" << is_bad->regex_string << "\"; skipping " << filepath << std::endl;
      continue;
    }

    for (const Matcher& m : blacklist) {
      if (m.IsMatch(filepath)) {
        is_bad = &m;
        break;
      }
    }
    if (is_bad) {
      std::cerr << "[" << i << "/" << (entries.size() - 1) << "] Failed blacklist check \"" << is_bad->regex_string << "\"; skipping " << filepath << std::endl;
      continue;
    }

    action(i, entries[i]);
  }
}

TEST_SUITE("Project");

TEST_CASE("Entry inference") {
  Project p;
  {
    Project::Entry e;
    e.args = { "arg1" };
    e.filename = "/a/b/c/d/bar.cc";
    p.entries.push_back(e);
  }
  {
    Project::Entry e;
    e.args = { "arg2" };
    e.filename = "/a/b/c/baz.cc";
    p.entries.push_back(e);
  }

  // Guess at same directory level, when there are parent directories.
  {
    optional<Project::Entry> entry = p.FindCompilationEntryForFile("/a/b/c/d/new.cc");
    REQUIRE(entry.has_value());
    REQUIRE(entry->args == std::vector<std::string>{ "arg1" });
  }

  // Guess at same directory level, when there are child directories.
  {
    optional<Project::Entry> entry = p.FindCompilationEntryForFile("/a/b/c/new.cc");
    REQUIRE(entry.has_value());
    REQUIRE(entry->args == std::vector<std::string>{ "arg2" });
  }

  // Guess at new directory (use the closest parent directory).
  {
    optional<Project::Entry> entry = p.FindCompilationEntryForFile("/a/b/c/new/new.cc");
    REQUIRE(entry.has_value());
    REQUIRE(entry->args == std::vector<std::string>{ "arg2" });
  }
}

TEST_SUITE_END();