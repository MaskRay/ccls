#include "project.h"

#include "match.h"
#include "libclangmm/Utility.h"
#include "platform.h"
#include "serializer.h"
#include "utils.h"

#include <clang-c/CXCompilationDatabase.h>
#include <doctest/doctest.h>

#include <iostream>
#include <sstream>
#include <unordered_set>
#include <vector>

struct CompileCommandsEntry {
  std::string directory;
  std::string file;
  std::string command;
  std::vector<std::string> args;
};
MAKE_REFLECT_STRUCT(CompileCommandsEntry, directory, file, command, args);

namespace {


static const char* kBlacklistMulti[] = {
  "-MF",
  "-Xclang"
};

// Blacklisted flags which are always removed from the command line.
static const char *kBlacklist[] = {
  "--param",
  "-M",
  "-MD",
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

  "-B",
  //"-f",
  //"-pipe",
  //"-W",
  // TODO: make sure we consume includes before stripping all path-like args.
  "/work/goma/gomacc",
  "../../third_party/llvm-build/Release+Asserts/bin/clang++",
  "-Wno-unused-lambda-capture",
  "/",
  "..",
  //"-stdlib=libc++"
};

// Arguments which are followed by a potentially relative path. We need to make
// all relative paths absolute, otherwise libclang will not resolve them.
const char* kPathArgs[] = {
  "-I",
  "-iquote",
  "-isystem",
  "--sysroot="
};

const char* kQuoteIncludeArgs[] = {
  "-iquote"
};
const char* kAngleIncludeArgs[] = {
  "-I",
  "-isystem"
};

bool ShouldAddToQuoteIncludes(const std::string& arg) {
  for (const char* flag_type : kQuoteIncludeArgs) {
    if (arg == flag_type)
      return true;
  }
  return false;
}
bool ShouldAddToAngleIncludes(const std::string& arg) {
  for (const char* flag_type : kAngleIncludeArgs) {
    if (StartsWith(arg, flag_type))
      return true;
  }
  return false;
}

// Returns true if we should use the C, not C++, language spec for the given
// file.
bool IsCFile(const std::string& path) {
  return EndsWith(path, ".c");
}

Project::Entry GetCompilationEntryFromCompileCommandEntry(
    std::unordered_set<std::string>& quote_includes, std::unordered_set<std::string>& angle_includes,
    const std::vector<std::string>& extra_flags, const CompileCommandsEntry& entry) {
  Project::Entry result;
  result.filename = NormalizePath(entry.file);

  bool make_next_flag_absolute = false;
  bool add_next_flag_quote = false;
  bool add_next_flag_angle = false;

  result.args.reserve(entry.args.size() + extra_flags.size());
  for (size_t i = 0; i < entry.args.size(); ++i) {
    std::string arg = entry.args[i];

    // If blacklist skip.
    if (std::any_of(std::begin(kBlacklistMulti), std::end(kBlacklistMulti), [&arg](const char* value) {
      return StartsWith(arg, value);
    })) {
      ++i;
      continue;
    }
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
      
      if (add_next_flag_quote)
        quote_includes.insert(arg);
      if (add_next_flag_angle)
        angle_includes.insert(arg);
      add_next_flag_quote = false;
      add_next_flag_angle = false;
    }

    // Update arg if it is a path.
    for (const char* flag_type : kPathArgs) {
      if (arg == flag_type) {
        make_next_flag_absolute = true;
        add_next_flag_quote = ShouldAddToQuoteIncludes(arg);
        add_next_flag_angle = ShouldAddToAngleIncludes(arg);
        break;
      }

      if (StartsWith(arg, flag_type)) {
        std::string path = arg.substr(strlen(flag_type));
        if (path.size() > 0 && path[0] != '/') {
          if (!entry.directory.empty())
            path = entry.directory + "/" + path;
          path = NormalizePath(path);

          arg = flag_type + path;
        }
        if (ShouldAddToQuoteIncludes(arg))
          quote_includes.insert(path);
        if (ShouldAddToAngleIncludes(arg))
          angle_includes.insert(path);
        break;
      }
    }

    result.args.push_back(arg);
  }

  // We don't do any special processing on user-given extra flags.
  for (const auto& flag : extra_flags)
    result.args.push_back(flag);

  // Clang does not have good hueristics for determining source language, we
  // should explicitly specify it.
  if (!AnyStartsWith(result.args, "-x")) {
    if (IsCFile(entry.file))
      result.args.push_back("-xc");
    else
      result.args.push_back("-xc++");
  }
  if (!AnyStartsWith(result.args, "-std=")) {
    if (IsCFile(entry.file))
      result.args.push_back("-std=c11");
    else
      result.args.push_back("-std=c++11");
  }

  return result;
}

/* TODO: Fix this function, it may be way faster than libclang's implementation.
std::vector<Project::Entry> LoadFromCompileCommandsJson(
    std::unordered_set<std::string>& quote_includes, std::unordered_set<std::string>& angle_includes,
    const std::vector<std::string>& extra_flags, const std::string& project_directory) {

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
  for (auto& entry : entries) {
    if (entry.args.empty() && !entry.command.empty())
      entry.args = SplitString(entry.command, " ");

    result.push_back(GetCompilationEntryFromCompileCommandEntry(quote_includes, angle_includes, extra_flags, entry));
  }
  return result;
}
*/

std::vector<Project::Entry> LoadFromDirectoryListing(
    std::unordered_set<std::string>& quote_includes, std::unordered_set<std::string>& angle_includes,
    const std::vector<std::string>& extra_flags, const std::string& project_directory) {
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
      CompileCommandsEntry e;
      e.file = NormalizePath(file);
      e.args = args;
      result.push_back(GetCompilationEntryFromCompileCommandEntry(quote_includes, angle_includes, extra_flags, e));
    }
  }

  return result;
}

std::vector<Project::Entry> LoadCompilationEntriesFromDirectory(
    std::unordered_set<std::string>& quote_includes, std::unordered_set<std::string>& angle_includes,
    const std::vector<std::string>& extra_flags, const std::string& project_directory) {
  // TODO: Figure out if this function or the clang one is faster.
  //return LoadFromCompileCommandsJson(extra_flags, project_directory);

  std::cerr << "Trying to load compile_commands.json" << std::endl;
  CXCompilationDatabase_Error cx_db_load_error;
  CXCompilationDatabase cx_db = clang_CompilationDatabase_fromDirectory(project_directory.c_str(), &cx_db_load_error);
  if (cx_db_load_error == CXCompilationDatabase_CanNotLoadDatabase) {
    std::cerr << "Unable to load compile_commands.json located at \"" << project_directory << "\"; using directory listing instead." << std::endl;
    return LoadFromDirectoryListing(quote_includes, angle_includes, extra_flags, project_directory);
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
    for (unsigned j = 0; j < num_args; ++j)
      entry.args.push_back(clang::ToString(clang_CompileCommand_getArg(cx_command, j)));

    result.push_back(GetCompilationEntryFromCompileCommandEntry(quote_includes, angle_includes, extra_flags, entry));
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
  std::unordered_set<std::string> unique_quote_includes;
  std::unordered_set<std::string> unique_angle_includes;

  entries = LoadCompilationEntriesFromDirectory(unique_quote_includes, unique_angle_includes, extra_flags, directory);

  quote_include_directories.assign(unique_quote_includes.begin(), unique_quote_includes.end());
  angle_include_directories.assign(unique_angle_includes.begin(), unique_angle_includes.end());

  for (std::string& path : quote_include_directories) {
    EnsureEndsInSlash(path);
    std::cerr << "quote_include_dir: " << path << std::endl;
  }
  for (std::string& path : angle_include_directories) {
    EnsureEndsInSlash(path);
    std::cerr << "angle_include_dir: " << path << std::endl;
  }

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

void Project::ForAllFilteredFiles(Config* config, std::function<void(int i, const Entry& entry)> action) {
  GroupMatch matcher(config->indexWhitelist, config->indexBlacklist);
  for (int i = 0; i < entries.size(); ++i) {
    const Project::Entry& entry = entries[i];
    std::string failure_reason;
    if (matcher.IsMatch(entry.filename, &failure_reason))
      action(i, entries[i]);
    else {
      if (config->logSkippedPathsForIndex) {
        std::stringstream output;
        output << '[' << (i + 1) << '/' << entries.size() << "] Failed " << failure_reason << "; skipping " << entry.filename << std::endl;
        std::cerr << output.str();
      }
    }
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
