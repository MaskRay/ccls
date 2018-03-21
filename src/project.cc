#include "project.h"

#include "cache_manager.h"
#include "clang_utils.h"
#include "language.h"
#include "match.h"
#include "platform.h"
#include "queue_manager.h"
#include "serializers/json.h"
#include "timer.h"
#include "utils.h"
#include "working_files.h"

#include <clang-c/CXCompilationDatabase.h>
#include <doctest/doctest.h>
#include <rapidjson/writer.h>
#include <loguru.hpp>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <unordered_set>
#include <vector>

extern bool gTestOutputMode;

struct CompileCommandsEntry {
  std::string directory;
  std::string file;
  std::string command;
  std::vector<std::string> args;
};
MAKE_REFLECT_STRUCT(CompileCommandsEntry, directory, file, command, args);

namespace {

bool g_disable_normalize_path_for_test = false;

std::string NormalizePathWithTestOptOut(const std::string& path) {
  if (g_disable_normalize_path_for_test) {
    // Add a & so we can test to verify a path is normalized.
    return "&" + path;
  }
  return NormalizePath(path);
}

bool IsUnixAbsolutePath(const std::string& path) {
  return !path.empty() && path[0] == '/';
}

bool IsWindowsAbsolutePath(const std::string& path) {
  auto is_drive_letter = [](char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
  };

  return path.size() > 3 && path[1] == ':' &&
         (path[2] == '/' || path[2] == '\\') && is_drive_letter(path[0]);
}

enum class ProjectMode { CompileCommandsJson, DotCquery, ExternalCommand };

struct ProjectConfig {
  std::unordered_set<std::string> quote_dirs;
  std::unordered_set<std::string> angle_dirs;
  std::vector<std::string> extra_flags;
  std::string project_dir;
  std::string resource_dir;
  ProjectMode mode = ProjectMode::CompileCommandsJson;
};

// TODO: See
// https://github.com/Valloric/ycmd/blob/master/ycmd/completers/cpp/flags.py.
std::vector<std::string> kBlacklistMulti = {
    "-MF", "-MT", "-MQ", "-o", "--serialize-diagnostics", "-Xclang"};

// Blacklisted flags which are always removed from the command line.
std::vector<std::string> kBlacklist = {
    "-c", "-MP", "-MD", "-MMD", "--fcolor-diagnostics",
};

// Arguments which are followed by a potentially relative path. We need to make
// all relative paths absolute, otherwise libclang will not resolve them.
std::vector<std::string> kPathArgs = {
    "-I",        "-iquote",        "-isystem",     "--sysroot=",
    "-isysroot", "-gcc-toolchain", "-include-pch", "-iframework",
    "-F",        "-imacros",       "-include",     "/I",
    "-idirafter"};

// Arguments which always require an absolute path, ie, clang -working-directory
// does not work as expected. Argument processing assumes that this is a subset
// of kPathArgs.
std::vector<std::string> kNormalizePathArgs = {"--sysroot="};

// Arguments whose path arguments should be injected into include dir lookup
// for #include completion.
std::vector<std::string> kQuoteIncludeArgs = {"-iquote"};
std::vector<std::string> kAngleIncludeArgs = {"-I", "/I", "-isystem"};

bool ShouldAddToQuoteIncludes(const std::string& arg) {
  return StartsWithAny(arg, kQuoteIncludeArgs);
}
bool ShouldAddToAngleIncludes(const std::string& arg) {
  return StartsWithAny(arg, kAngleIncludeArgs);
}

LanguageId SourceFileLanguage(const std::string& path) {
  if (EndsWith(path, ".c"))
    return LanguageId::C;
  else if (EndsWith(path, ".cpp") || EndsWith(path, ".cc"))
    return LanguageId::Cpp;
  else if (EndsWith(path, ".mm"))
    return LanguageId::ObjCpp;
  else if (EndsWith(path, ".m"))
    return LanguageId::ObjC;
  return LanguageId::Unknown;
}

Project::Entry GetCompilationEntryFromCompileCommandEntry(
    Config* init_opts,
    ProjectConfig* config,
    const CompileCommandsEntry& entry) {
  auto cleanup_maybe_relative_path = [&](const std::string& path) {
    // TODO/FIXME: Normalization will fail for paths that do not exist. Should
    // it return an optional<std::string>?
    assert(!path.empty());
    if (entry.directory.empty() || IsUnixAbsolutePath(path) ||
        IsWindowsAbsolutePath(path)) {
      // We still want to normalize, as the path may contain .. characters.
      return NormalizePathWithTestOptOut(path);
    }
    if (EndsWith(entry.directory, "/"))
      return NormalizePathWithTestOptOut(entry.directory + path);
    return NormalizePathWithTestOptOut(entry.directory + "/" + path);
  };

  Project::Entry result;
  result.filename = NormalizePathWithTestOptOut(entry.file);
  const std::string base_name = GetBaseName(entry.file);

  // Expand %c %cpp %clang
  std::vector<std::string> args;
  const LanguageId lang = SourceFileLanguage(entry.file);
  for (const std::string& arg : entry.args) {
    if (arg.compare(0, 3, "%c ") == 0) {
      if (lang == LanguageId::C)
        args.push_back(arg.substr(3));
    } else if (arg.compare(0, 5, "%cpp ") == 0) {
      if (lang == LanguageId::Cpp)
        args.push_back(arg.substr(5));
    } else if (arg == "%clang") {
      args.push_back(lang == LanguageId::Cpp ? "clang++" : "clang");
    } else {
      args.push_back(arg);
    }
  }
  if (args.empty())
    return result;

  std::string first_arg = args[0];
  // Windows' filesystem is not case sensitive, so we compare only
  // the lower case variant.
  std::transform(first_arg.begin(), first_arg.end(), first_arg.begin(),
                 tolower);
  bool clang_cl = strstr(first_arg.c_str(), "clang-cl") ||
                  strstr(first_arg.c_str(), "cl.exe");
  // Clang only cares about the last --driver-mode flag, so the loop
  // iterates in reverse to find the last one as soon as possible
  // in case of multiple --driver-mode flags.
  for (int i = args.size() - 1; i >= 0; --i) {
    if (strstr(args[i].c_str(), "--dirver-mode=")) {
      clang_cl = clang_cl || strstr(args[i].c_str(), "--driver-mode=cl");
      break;
    }
  }
  size_t i = 1;

  // If |compilationDatabaseCommand| is specified, the external command provides
  // us the JSON compilation database which should be strict. We should do very
  // little processing on |args|.
  if (config->mode != ProjectMode::ExternalCommand && !clang_cl) {
    // Strip all arguments consisting the compiler command,
    // as there may be non-compiler related commands beforehand,
    // ie, compiler schedular such as goma. This allows correct parsing for
    // command lines like "goma clang -c foo".
    std::string::size_type dot;
    while (i < args.size() && args[i][0] != '-' &&
           // Do not skip over main source filename
           NormalizePathWithTestOptOut(args[i]) != result.filename &&
           // There may be other filenames (e.g. more than one source filenames)
           // preceding main source filename. We use a heuristic here. `.` may
           // occur in both command names and source filenames. If `.` occurs in
           // the last 4 bytes of args[i] and not followed by a digit, e.g.
           // .c .cpp, We take it as a source filename. Others (like ./a/b/goma
           // clang-4.0) are seen as commands.
           ((dot = args[i].rfind('.')) == std::string::npos ||
            dot + 4 < args[i].size() || isdigit(args[i][dot + 1]) ||
            !args[i].compare(dot + 1, 3, "exe")))
      ++i;
  }
  // Compiler driver.
  result.args.push_back(args[i - 1]);

  // Add -working-directory if not provided.
  if (!AnyStartsWith(args, "-working-directory"))
    result.args.emplace_back("-working-directory=" + entry.directory);

  if (!gTestOutputMode) {
    std::vector<const char*> platform = GetPlatformClangArguments();
    for (auto arg : platform)
      result.args.push_back(arg);
  }

  bool next_flag_is_path = false;
  bool add_next_flag_to_quote_dirs = false;
  bool add_next_flag_to_angle_dirs = false;

  // Note that when processing paths, some arguments support multiple forms, ie,
  // {"-Ifoo"} or {"-I", "foo"}.  Support both styles.

  result.args.reserve(args.size() + config->extra_flags.size());
  for (; i < args.size(); ++i) {
    std::string arg = args[i];

    // If blacklist skip.
    if (!next_flag_is_path) {
      if (StartsWithAny(arg, kBlacklistMulti)) {
        ++i;
        continue;
      }
      if (StartsWithAny(arg, kBlacklist))
        continue;
    }

    // Finish processing path for the previous argument, which was a switch.
    // {"-I", "foo"} style.
    if (next_flag_is_path) {
      std::string normalized_arg = cleanup_maybe_relative_path(arg);
      if (add_next_flag_to_quote_dirs)
        config->quote_dirs.insert(normalized_arg);
      if (add_next_flag_to_angle_dirs)
        config->angle_dirs.insert(normalized_arg);
      if (clang_cl)
        arg = normalized_arg;

      next_flag_is_path = false;
      add_next_flag_to_quote_dirs = false;
      add_next_flag_to_angle_dirs = false;
    } else {
      // Check to see if arg is a path and needs to be updated.
      for (const std::string& flag_type : kPathArgs) {
        // {"-I", "foo"} style.
        if (arg == flag_type) {
          next_flag_is_path = true;
          add_next_flag_to_quote_dirs = ShouldAddToQuoteIncludes(arg);
          add_next_flag_to_angle_dirs = ShouldAddToAngleIncludes(arg);
          break;
        }

        // {"-Ifoo"} style.
        if (StartsWith(arg, flag_type)) {
          std::string path = arg.substr(flag_type.size());
          assert(!path.empty());
          path = cleanup_maybe_relative_path(path);
          if (clang_cl || StartsWithAny(arg, kNormalizePathArgs))
            arg = flag_type + path;
          if (ShouldAddToQuoteIncludes(flag_type))
            config->quote_dirs.insert(path);
          if (ShouldAddToAngleIncludes(flag_type))
            config->angle_dirs.insert(path);
          break;
        }
      }

      // This is most likely the file path we will be passing to clang. The
      // path needs to be absolute, otherwise clang_codeCompleteAt is extremely
      // slow. See
      // https://github.com/cquery-project/cquery/commit/af63df09d57d765ce12d40007bf56302a0446678.
      if (EndsWith(arg, base_name))
        arg = cleanup_maybe_relative_path(arg);
      // TODO Exclude .a .o to make link command in compile_commands.json work.
      // Also, clang_parseTranslationUnit2FullArgv does not seem to accept
      // multiple source filenames.
      else if (EndsWith(arg, ".a") || EndsWith(arg, ".o"))
        continue;
    }

    result.args.push_back(arg);
  }

  // We don't do any special processing on user-given extra flags.
  for (const auto& flag : config->extra_flags)
    result.args.push_back(flag);

  // Add -resource-dir so clang can correctly resolve system includes like
  // <cstddef>
  if (!AnyStartsWith(result.args, "-resource-dir"))
    result.args.push_back("-resource-dir=" + config->resource_dir);

  // There could be a clang version mismatch between what the project uses and
  // what cquery uses. Make sure we do not emit warnings for mismatched options.
  if (!AnyStartsWith(result.args, "-Wno-unknown-warning-option"))
    result.args.push_back("-Wno-unknown-warning-option");

  // Using -fparse-all-comments enables documentation in the indexer and in
  // code completion.
  if (init_opts->index.comments > 1 &&
      !AnyStartsWith(result.args, "-fparse-all-comments")) {
    result.args.push_back("-fparse-all-comments");
  }

  return result;
}

std::vector<std::string> ReadCompilerArgumentsFromFile(
    const std::string& path) {
  std::vector<std::string> args;
  for (std::string line : ReadLinesWithEnding(path)) {
    TrimInPlace(line);
    if (line.empty() || StartsWith(line, "#"))
      continue;
    args.push_back(line);
  }
  return args;
}

std::vector<Project::Entry> LoadFromDirectoryListing(Config* init_opts,
                                                     ProjectConfig* config) {
  std::vector<Project::Entry> result;
  config->mode = ProjectMode::DotCquery;
  LOG_IF_S(WARNING, !FileExists(config->project_dir + "/.cquery") &&
                        config->extra_flags.empty())
      << "cquery has no clang arguments. Considering adding either a "
         "compile_commands.json or .cquery file. See the cquery README for "
         "more information.";

  std::unordered_map<std::string, std::vector<std::string>> folder_args;
  std::vector<std::string> files;

  GetFilesInFolder(config->project_dir, true /*recursive*/,
                   true /*add_folder_to_path*/,
                   [&folder_args, &files](const std::string& path) {
                     if (SourceFileLanguage(path) != LanguageId::Unknown) {
                       files.push_back(path);
                     } else if (GetBaseName(path) == ".cquery") {
                       LOG_S(INFO) << "Using .cquery arguments from " << path;
                       folder_args.emplace(GetDirName(path),
                                           ReadCompilerArgumentsFromFile(path));
                     }
                   });

  const auto& project_dir_args = folder_args[config->project_dir];
  LOG_IF_S(INFO, !project_dir_args.empty())
      << "Using .cquery arguments " << StringJoin(project_dir_args);

  auto GetCompilerArgumentForFile = [&config,
                                     &folder_args](const std::string& path) {
    for (std::string cur = GetDirName(path);; cur = GetDirName(cur)) {
      auto it = folder_args.find(cur);
      if (it != folder_args.end())
        return it->second;
      std::string normalized = NormalizePath(cur);
      // Break if outside of the project root.
      if (normalized.size() <= config->project_dir.size() ||
          normalized.compare(0, config->project_dir.size(),
                             config->project_dir) != 0)
        break;
    }
    return folder_args[config->project_dir];
  };

  for (const std::string& file : files) {
    CompileCommandsEntry e;
    e.directory = config->project_dir;
    e.file = file;
    e.args = GetCompilerArgumentForFile(file);
    if (e.args.empty())
      e.args.push_back("%clang");  // Add a Dummy.
    e.args.push_back(e.file);
    result.push_back(
        GetCompilationEntryFromCompileCommandEntry(init_opts, config, e));
  }

  return result;
}

std::vector<Project::Entry> LoadCompilationEntriesFromDirectory(
    Config* config,
    ProjectConfig* project,
    const std::string& opt_compilation_db_dir) {
  // If there is a .cquery file always load using directory listing.
  if (FileExists(project->project_dir + ".cquery"))
    return LoadFromDirectoryListing(config, project);

  // If |compilationDatabaseCommand| is specified, execute it to get the compdb.
  std::string comp_db_dir;
  if (config->compilationDatabaseCommand.empty()) {
    project->mode = ProjectMode::CompileCommandsJson;
    // Try to load compile_commands.json, but fallback to a project listing.
    comp_db_dir = opt_compilation_db_dir.empty() ? project->project_dir
                                                 : opt_compilation_db_dir;
  } else {
    project->mode = ProjectMode::ExternalCommand;
#ifdef _WIN32
    // TODO
#else
    char tmpdir[] = "/tmp/cquery-compdb-XXXXXX";
    if (!mkdtemp(tmpdir))
      return {};
    comp_db_dir = tmpdir;
    rapidjson::StringBuffer input;
    rapidjson::Writer<rapidjson::StringBuffer> writer(input);
    JsonWriter json_writer(&writer);
    Reflect(json_writer, *config);
    std::string contents = GetExternalCommandOutput(
        std::vector<std::string>{config->compilationDatabaseCommand,
                                 project->project_dir},
        input.GetString());
    std::ofstream(comp_db_dir + "/compile_commands.json") << contents;
#endif
  }

  LOG_S(INFO) << "Trying to load compile_commands.json";
  CXCompilationDatabase_Error cx_db_load_error;
  CXCompilationDatabase cx_db = clang_CompilationDatabase_fromDirectory(
      comp_db_dir.c_str(), &cx_db_load_error);
  if (!config->compilationDatabaseCommand.empty()) {
#ifdef _WIN32
  // TODO
#else
    unlink((comp_db_dir + "/compile_commands.json").c_str());
    rmdir(comp_db_dir.c_str());
#endif
  }

  if (cx_db_load_error == CXCompilationDatabase_CanNotLoadDatabase) {
    LOG_S(INFO) << "Unable to load compile_commands.json located at \""
                << comp_db_dir << "\"; using directory listing instead.";
    return LoadFromDirectoryListing(config, project);
  }

  Timer clang_time;
  Timer our_time;
  clang_time.Pause();
  our_time.Pause();

  clang_time.Resume();
  CXCompileCommands cx_commands =
      clang_CompilationDatabase_getAllCompileCommands(cx_db);
  unsigned int num_commands = clang_CompileCommands_getSize(cx_commands);
  clang_time.Pause();

  std::vector<Project::Entry> result;
  for (unsigned int i = 0; i < num_commands; i++) {
    clang_time.Resume();
    CXCompileCommand cx_command =
        clang_CompileCommands_getCommand(cx_commands, i);

    std::string directory =
        ToString(clang_CompileCommand_getDirectory(cx_command));
    std::string relative_filename =
        ToString(clang_CompileCommand_getFilename(cx_command));

    unsigned num_args = clang_CompileCommand_getNumArgs(cx_command);
    CompileCommandsEntry entry;
    entry.args.reserve(num_args);
    for (unsigned j = 0; j < num_args; ++j) {
      entry.args.push_back(
          ToString(clang_CompileCommand_getArg(cx_command, j)));
    }
    clang_time.Pause();  // TODO: don't call ToString in this block.
    // LOG_S(INFO) << "Got args " << StringJoin(entry.args);

    our_time.Resume();
    entry.directory = directory;
    std::string absolute_filename;
    if (IsUnixAbsolutePath(relative_filename) ||
        IsWindowsAbsolutePath(relative_filename))
      absolute_filename = relative_filename;
    else
      absolute_filename = directory + "/" + relative_filename;
    entry.file = NormalizePathWithTestOptOut(absolute_filename);

    result.push_back(
        GetCompilationEntryFromCompileCommandEntry(config, project, entry));
    our_time.Pause();
  }

  clang_time.Resume();
  clang_CompileCommands_dispose(cx_commands);
  clang_CompilationDatabase_dispose(cx_db);
  clang_time.Pause();

  clang_time.ResetAndPrint("compile_commands.json clang time");
  our_time.ResetAndPrint("compile_commands.json our time");
  return result;
}

// Computes a score based on how well |a| and |b| match. This is used for
// argument guessing.
int ComputeGuessScore(const std::string& a, const std::string& b) {
  const int kMatchPrefixWeight = 100;
  const int kMismatchDirectoryWeight = 100;
  const int kMatchPostfixWeight = 1;

  int score = 0;
  size_t i = 0;

  // Increase score based on matching prefix.
  for (i = 0; i < a.size() && i < b.size(); ++i) {
    if (a[i] != b[i])
      break;
    score += kMatchPrefixWeight;
  }

  // Reduce score based on mismatched directory distance.
  for (size_t j = i; j < a.size(); ++j) {
    if (a[j] == '/')
      score -= kMismatchDirectoryWeight;
  }
  for (size_t j = i; j < b.size(); ++j) {
    if (b[j] == '/')
      score -= kMismatchDirectoryWeight;
  }

  // Increase score based on common ending. Don't increase as much as matching
  // prefix or directory distance.
  for (size_t offset = 1; offset <= a.size() && offset <= b.size(); ++offset) {
    if (a[a.size() - offset] != b[b.size() - offset])
      break;
    score += kMatchPostfixWeight;
  }

  return score;
}

}  // namespace

void Project::Load(Config* config, const std::string& root_directory) {
  // Load data.
  ProjectConfig project;
  project.extra_flags = config->extraClangArguments;
  project.project_dir = root_directory;
  project.resource_dir = config->resourceDirectory;
  entries = LoadCompilationEntriesFromDirectory(
      config, &project, config->compilationDatabaseDirectory);

  // Cleanup / postprocess include directories.
  quote_include_directories.assign(project.quote_dirs.begin(),
                                   project.quote_dirs.end());
  angle_include_directories.assign(project.angle_dirs.begin(),
                                   project.angle_dirs.end());
  for (std::string& path : quote_include_directories) {
    EnsureEndsInSlash(path);
    LOG_S(INFO) << "quote_include_dir: " << path;
  }
  for (std::string& path : angle_include_directories) {
    EnsureEndsInSlash(path);
    LOG_S(INFO) << "angle_include_dir: " << path;
  }

  // Setup project entries.
  absolute_path_to_entry_index_.resize(entries.size());
  for (int i = 0; i < entries.size(); ++i)
    absolute_path_to_entry_index_[entries[i].filename] = i;
}

void Project::SetFlagsForFile(
    const std::vector<std::string>& flags,
    const std::string& path) {
  auto it = absolute_path_to_entry_index_.find(path);
  if (it != absolute_path_to_entry_index_.end()) {
    // The entry already exists in the project, just set the flags.
    this->entries[it->second].args = flags;
  } else {
    // Entry wasn't found, so we create a new one.
    Entry entry;
    entry.is_inferred = false;
    entry.filename = path;
    entry.args = flags;
    this->entries.emplace_back(entry);
  }
}

Project::Entry Project::FindCompilationEntryForFile(
    const std::string& filename) {
  auto it = absolute_path_to_entry_index_.find(filename);
  if (it != absolute_path_to_entry_index_.end())
    return entries[it->second];

  // We couldn't find the file. Try to infer it.
  // TODO: Cache inferred file in a separate array (using a lock or similar)
  Entry* best_entry = nullptr;
  int best_score = std::numeric_limits<int>::min();
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
  if (!best_entry) {
    result.args.push_back("%clang");
    result.args.push_back(filename);
  } else {
    result.args = best_entry->args;

    // |best_entry| probably has its own path in the arguments. We need to remap
    // that path to the new filename.
    std::string best_entry_base_name = GetBaseName(best_entry->filename);
    for (std::string& arg : result.args) {
      if (arg == best_entry->filename ||
          GetBaseName(arg) == best_entry_base_name) {
        arg = filename;
      }
    }
  }

  return result;
}

void Project::ForAllFilteredFiles(
    Config* config,
    std::function<void(int i, const Entry& entry)> action) {
  GroupMatch matcher(config->index.whitelist, config->index.blacklist);
  for (int i = 0; i < entries.size(); ++i) {
    const Project::Entry& entry = entries[i];
    std::string failure_reason;
    if (matcher.IsMatch(entry.filename, &failure_reason))
      action(i, entries[i]);
    else {
      if (config->index.logSkippedPaths) {
        LOG_S(INFO) << "[" << i + 1 << "/" << entries.size() << "]: Failed "
                    << failure_reason << "; skipping " << entry.filename;
      }
    }
  }
}

void Project::Index(Config* config,
                    QueueManager* queue,
                    WorkingFiles* wfiles,
                    lsRequestId id) {
  ForAllFilteredFiles(config, [&](int i, const Project::Entry& entry) {
    optional<std::string> content = ReadContent(entry.filename);
    if (!content) {
      LOG_S(ERROR) << "When loading project, canont read file "
                   << entry.filename;
      return;
    }
    bool is_interactive = wfiles->GetFileByFilename(entry.filename) != nullptr;
    queue->index_request.PushBack(
        Index_Request(entry.filename, entry.args, is_interactive, *content,
                      ICacheManager::Make(config), id));
  });
}

TEST_SUITE("Project") {
  void CheckFlags(const std::string& directory, const std::string& file,
                  std::vector<std::string> raw,
                  std::vector<std::string> expected) {
    g_disable_normalize_path_for_test = true;
    gTestOutputMode = true;

    Config config;
    ProjectConfig project;
    project.project_dir = "/w/c/s/";
    project.resource_dir = "/w/resource_dir/";

    CompileCommandsEntry entry;
    entry.directory = directory;
    entry.args = raw;
    entry.file = file;
    Project::Entry result =
        GetCompilationEntryFromCompileCommandEntry(&config, &project, entry);

    if (result.args != expected) {
      std::cout << "Raw:      " << StringJoin(raw) << std::endl;
      std::cout << "Expected: " << StringJoin(expected) << std::endl;
      std::cout << "Actual:   " << StringJoin(result.args) << std::endl;
    }
    for (int i = 0; i < std::min(result.args.size(), expected.size()); ++i) {
      if (result.args[i] != expected[i]) {
        std::cout << std::endl;
        std::cout << "mismatch at " << i << std::endl;
        std::cout << "  expected: " << expected[i] << std::endl;
        std::cout << "  actual:   " << result.args[i] << std::endl;
      }
    }
    REQUIRE(result.args == expected);
  }

  void CheckFlags(std::vector<std::string> raw,
                  std::vector<std::string> expected) {
    CheckFlags("/dir/", "file.cc", raw, expected);
  }

  TEST_CASE("strip meta-compiler invocations") {
    CheckFlags(
        /* raw */ {"clang", "-lstdc++", "myfile.cc"},
        /* expected */
        {"clang", "-working-directory=/dir/", "-lstdc++", "&/dir/myfile.cc",
         "-resource-dir=/w/resource_dir/", "-Wno-unknown-warning-option",
         "-fparse-all-comments"});

    CheckFlags(
        /* raw */ {"clang.exe"},
        /* expected */
        {"clang.exe", "-working-directory=/dir/",
         "-resource-dir=/w/resource_dir/", "-Wno-unknown-warning-option",
         "-fparse-all-comments"});

    CheckFlags(
        /* raw */ {"goma", "clang"},
        /* expected */
        {"clang", "-working-directory=/dir/", "-resource-dir=/w/resource_dir/",
         "-Wno-unknown-warning-option", "-fparse-all-comments"});

    CheckFlags(
        /* raw */ {"goma", "clang", "--foo"},
        /* expected */
        {"clang", "-working-directory=/dir/", "--foo",
         "-resource-dir=/w/resource_dir/", "-Wno-unknown-warning-option",
         "-fparse-all-comments"});
  }

  TEST_CASE("Windows path normalization") {
    CheckFlags("E:/workdir", "E:/workdir/bar.cc", /* raw */ {"clang", "bar.cc"},
               /* expected */
               {"clang", "-working-directory=E:/workdir", "&E:/workdir/bar.cc",
                "-resource-dir=/w/resource_dir/", "-Wno-unknown-warning-option",
                "-fparse-all-comments"});

    CheckFlags("E:/workdir", "E:/workdir/bar.cc",
               /* raw */ {"clang", "E:/workdir/bar.cc"},
               /* expected */
               {"clang", "-working-directory=E:/workdir", "&E:/workdir/bar.cc",
                "-resource-dir=/w/resource_dir/", "-Wno-unknown-warning-option",
                "-fparse-all-comments"});

    CheckFlags("E:/workdir", "E:/workdir/bar.cc",
               /* raw */ {"clang-cl.exe", "/I./test", "E:/workdir/bar.cc"},
               /* expected */
               {"clang-cl.exe", "-working-directory=E:/workdir",
                "/I&E:/workdir/./test", "&E:/workdir/bar.cc",
                "-resource-dir=/w/resource_dir/", "-Wno-unknown-warning-option",
                "-fparse-all-comments"});

    CheckFlags("E:/workdir", "E:/workdir/bar.cc",
               /* raw */
               {"cl.exe", "/I../third_party/test/include", "E:/workdir/bar.cc"},
               /* expected */
               {"cl.exe", "-working-directory=E:/workdir",
                "/I&E:/workdir/../third_party/test/include",
                "&E:/workdir/bar.cc", "-resource-dir=/w/resource_dir/",
                "-Wno-unknown-warning-option", "-fparse-all-comments"});
  }

  TEST_CASE("Path in args") {
    CheckFlags("/home/user", "/home/user/foo/bar.c",
               /* raw */ {"cc", "-O0", "foo/bar.c"},
               /* expected */
               {"cc", "-working-directory=/home/user", "-O0",
                "&/home/user/foo/bar.c", "-resource-dir=/w/resource_dir/",
                "-Wno-unknown-warning-option", "-fparse-all-comments"});
  }

  TEST_CASE("Implied binary") {
    CheckFlags("/home/user", "/home/user/foo/bar.cc",
               /* raw */ {"clang", "-DDONT_IGNORE_ME"},
               /* expected */
               {"clang", "-working-directory=/home/user", "-DDONT_IGNORE_ME",
                "-resource-dir=/w/resource_dir/", "-Wno-unknown-warning-option",
                "-fparse-all-comments"});
  }

  // Checks flag parsing for a random chromium file in comparison to what
  // YouCompleteMe fetches.
  TEST_CASE("ycm") {
    CheckFlags(
        "/w/c/s/out/Release", "../../ash/login/lock_screen_sanity_unittest.cc",

        /* raw */
        {
            "/work/goma/gomacc",
            "../../third_party/llvm-build/Release+Asserts/bin/clang++",
            "-MMD",
            "-MF",
            "obj/ash/ash_unittests/lock_screen_sanity_unittest.o.d",
            "-DV8_DEPRECATION_WARNINGS",
            "-DDCHECK_ALWAYS_ON=1",
            "-DUSE_UDEV",
            "-DUSE_AURA=1",
            "-DUSE_NSS_CERTS=1",
            "-DUSE_OZONE=1",
            "-DFULL_SAFE_BROWSING",
            "-DSAFE_BROWSING_CSD",
            "-DSAFE_BROWSING_DB_LOCAL",
            "-DCHROMIUM_BUILD",
            "-DFIELDTRIAL_TESTING_ENABLED",
            "-D_FILE_OFFSET_BITS=64",
            "-D_LARGEFILE_SOURCE",
            "-D_LARGEFILE64_SOURCE",
            "-DCR_CLANG_REVISION=\"313786-1\"",
            "-D__STDC_CONSTANT_MACROS",
            "-D__STDC_FORMAT_MACROS",
            "-DCOMPONENT_BUILD",
            "-DOS_CHROMEOS",
            "-DNDEBUG",
            "-DNVALGRIND",
            "-DDYNAMIC_ANNOTATIONS_ENABLED=0",
            "-DGL_GLEXT_PROTOTYPES",
            "-DUSE_GLX",
            "-DUSE_EGL",
            "-DANGLE_ENABLE_RELEASE_ASSERTS",
            "-DTOOLKIT_VIEWS=1",
            "-DGTEST_API_=",
            "-DGTEST_HAS_POSIX_RE=0",
            "-DGTEST_LANG_CXX11=1",
            "-DUNIT_TEST",
            "-DUSING_V8_SHARED",
            "-DU_USING_ICU_NAMESPACE=0",
            "-DU_ENABLE_DYLOAD=0",
            "-DICU_UTIL_DATA_IMPL=ICU_UTIL_DATA_FILE",
            "-DUCHAR_TYPE=uint16_t",
            "-DGOOGLE_PROTOBUF_NO_RTTI",
            "-DGOOGLE_PROTOBUF_NO_STATIC_INITIALIZER",
            "-DHAVE_PTHREAD",
            "-DPROTOBUF_USE_DLLS",
            "-DBORINGSSL_SHARED_LIBRARY",
            "-DSK_IGNORE_LINEONLY_AA_CONVEX_PATH_OPTS",
            "-DSK_HAS_PNG_LIBRARY",
            "-DSK_HAS_WEBP_LIBRARY",
            "-DSK_HAS_JPEG_LIBRARY",
            "-DSKIA_DLL",
            "-DGR_GL_IGNORE_ES3_MSAA=0",
            "-DSK_SUPPORT_GPU=1",
            "-DMESA_EGL_NO_X11_HEADERS",
            "-I../..",
            "-Igen",
            "-I../../third_party/libwebp/src",
            "-I../../third_party/khronos",
            "-I../../gpu",
            "-I../../third_party/googletest/src/googletest/include",
            "-I../../third_party/WebKit",
            "-Igen/third_party/WebKit",
            "-I../../v8/include",
            "-Igen/v8/include",
            "-I../../third_party/icu/source/common",
            "-I../../third_party/icu/source/i18n",
            "-I../../third_party/protobuf/src",
            "-Igen/protoc_out",
            "-I../../third_party/protobuf/src",
            "-I../../third_party/boringssl/src/include",
            "-I../../build/linux/debian_jessie_amd64-sysroot/usr/include/nss",
            "-I../../build/linux/debian_jessie_amd64-sysroot/usr/include/nspr",
            "-I../../skia/config",
            "-I../../skia/ext",
            "-I../../third_party/skia/include/c",
            "-I../../third_party/skia/include/config",
            "-I../../third_party/skia/include/core",
            "-I../../third_party/skia/include/effects",
            "-I../../third_party/skia/include/encode",
            "-I../../third_party/skia/include/gpu",
            "-I../../third_party/skia/include/images",
            "-I../../third_party/skia/include/lazy",
            "-I../../third_party/skia/include/pathops",
            "-I../../third_party/skia/include/pdf",
            "-I../../third_party/skia/include/pipe",
            "-I../../third_party/skia/include/ports",
            "-I../../third_party/skia/include/utils",
            "-I../../third_party/skia/third_party/vulkan",
            "-I../../third_party/skia/include/codec",
            "-I../../third_party/skia/src/gpu",
            "-I../../third_party/skia/src/sksl",
            "-I../../third_party/ced/src",
            "-I../../third_party/mesa/src/include",
            "-I../../third_party/libwebm/source",
            "-Igen",
            "-I../../build/linux/debian_jessie_amd64-sysroot/usr/include/"
            "dbus-1.0",
            "-I../../build/linux/debian_jessie_amd64-sysroot/usr/lib/"
            "x86_64-linux-gnu/dbus-1.0/include",
            "-I../../third_party/googletest/custom",
            "-I../../third_party/googletest/src/googlemock/include",
            "-fno-strict-aliasing",
            "-Wno-builtin-macro-redefined",
            "-D__DATE__=",
            "-D__TIME__=",
            "-D__TIMESTAMP__=",
            "-funwind-tables",
            "-fPIC",
            "-pipe",
            "-B../../third_party/binutils/Linux_x64/Release/bin",
            "-pthread",
            "-fcolor-diagnostics",
            "-no-canonical-prefixes",
            "-m64",
            "-march=x86-64",
            "-Wall",
            "-Werror",
            "-Wextra",
            "-Wno-missing-field-initializers",
            "-Wno-unused-parameter",
            "-Wno-c++11-narrowing",
            "-Wno-covered-switch-default",
            "-Wno-unneeded-internal-declaration",
            "-Wno-inconsistent-missing-override",
            "-Wno-undefined-var-template",
            "-Wno-nonportable-include-path",
            "-Wno-address-of-packed-member",
            "-Wno-unused-lambda-capture",
            "-Wno-user-defined-warnings",
            "-Wno-enum-compare-switch",
            "-Wno-tautological-unsigned-zero-compare",
            "-Wno-null-pointer-arithmetic",
            "-Wno-tautological-unsigned-enum-zero-compare",
            "-O2",
            "-fno-ident",
            "-fdata-sections",
            "-ffunction-sections",
            "-fno-omit-frame-pointer",
            "-g0",
            "-fvisibility=hidden",
            "-Xclang",
            "-load",
            "-Xclang",
            "../../third_party/llvm-build/Release+Asserts/lib/"
            "libFindBadConstructs.so",
            "-Xclang",
            "-add-plugin",
            "-Xclang",
            "find-bad-constructs",
            "-Xclang",
            "-plugin-arg-find-bad-constructs",
            "-Xclang",
            "check-auto-raw-pointer",
            "-Xclang",
            "-plugin-arg-find-bad-constructs",
            "-Xclang",
            "check-ipc",
            "-Wheader-hygiene",
            "-Wstring-conversion",
            "-Wtautological-overlap-compare",
            "-Wno-header-guard",
            "-std=gnu++14",
            "-fno-rtti",
            "-nostdinc++",
            "-isystem../../buildtools/third_party/libc++/trunk/include",
            "-isystem../../buildtools/third_party/libc++abi/trunk/include",
            "--sysroot=../../build/linux/debian_jessie_amd64-sysroot",
            "-fno-exceptions",
            "-fvisibility-inlines-hidden",
            "-c",
            "../../ash/login/ui/lock_screen_sanity_unittest.cc",
            "-o",
            "obj/ash/ash_unittests/lock_screen_sanity_unittest.o",
        },

        /* expected */
        {"../../third_party/llvm-build/Release+Asserts/bin/clang++",
         "-working-directory=/w/c/s/out/Release",
         "-DV8_DEPRECATION_WARNINGS",
         "-DDCHECK_ALWAYS_ON=1",
         "-DUSE_UDEV",
         "-DUSE_AURA=1",
         "-DUSE_NSS_CERTS=1",
         "-DUSE_OZONE=1",
         "-DFULL_SAFE_BROWSING",
         "-DSAFE_BROWSING_CSD",
         "-DSAFE_BROWSING_DB_LOCAL",
         "-DCHROMIUM_BUILD",
         "-DFIELDTRIAL_TESTING_ENABLED",
         "-D_FILE_OFFSET_BITS=64",
         "-D_LARGEFILE_SOURCE",
         "-D_LARGEFILE64_SOURCE",
         "-DCR_CLANG_REVISION=\"313786-1\"",
         "-D__STDC_CONSTANT_MACROS",
         "-D__STDC_FORMAT_MACROS",
         "-DCOMPONENT_BUILD",
         "-DOS_CHROMEOS",
         "-DNDEBUG",
         "-DNVALGRIND",
         "-DDYNAMIC_ANNOTATIONS_ENABLED=0",
         "-DGL_GLEXT_PROTOTYPES",
         "-DUSE_GLX",
         "-DUSE_EGL",
         "-DANGLE_ENABLE_RELEASE_ASSERTS",
         "-DTOOLKIT_VIEWS=1",
         "-DGTEST_API_=",
         "-DGTEST_HAS_POSIX_RE=0",
         "-DGTEST_LANG_CXX11=1",
         "-DUNIT_TEST",
         "-DUSING_V8_SHARED",
         "-DU_USING_ICU_NAMESPACE=0",
         "-DU_ENABLE_DYLOAD=0",
         "-DICU_UTIL_DATA_IMPL=ICU_UTIL_DATA_FILE",
         "-DUCHAR_TYPE=uint16_t",
         "-DGOOGLE_PROTOBUF_NO_RTTI",
         "-DGOOGLE_PROTOBUF_NO_STATIC_INITIALIZER",
         "-DHAVE_PTHREAD",
         "-DPROTOBUF_USE_DLLS",
         "-DBORINGSSL_SHARED_LIBRARY",
         "-DSK_IGNORE_LINEONLY_AA_CONVEX_PATH_OPTS",
         "-DSK_HAS_PNG_LIBRARY",
         "-DSK_HAS_WEBP_LIBRARY",
         "-DSK_HAS_JPEG_LIBRARY",
         "-DSKIA_DLL",
         "-DGR_GL_IGNORE_ES3_MSAA=0",
         "-DSK_SUPPORT_GPU=1",
         "-DMESA_EGL_NO_X11_HEADERS",
         "-I../..",
         "-Igen",
         "-I../../third_party/libwebp/src",
         "-I../../third_party/khronos",
         "-I../../gpu",
         "-I../../third_party/googletest/src/googletest/"
         "include",
         "-I../../third_party/WebKit",
         "-Igen/third_party/WebKit",
         "-I../../v8/include",
         "-Igen/v8/include",
         "-I../../third_party/icu/source/common",
         "-I../../third_party/icu/source/i18n",
         "-I../../third_party/protobuf/src",
         "-Igen/protoc_out",
         "-I../../third_party/protobuf/src",
         "-I../../third_party/boringssl/src/include",
         "-I../../build/linux/debian_jessie_amd64-sysroot/"
         "usr/include/nss",
         "-I../../build/linux/debian_jessie_amd64-sysroot/"
         "usr/include/nspr",
         "-I../../skia/config",
         "-I../../skia/ext",
         "-I../../third_party/skia/include/c",
         "-I../../third_party/skia/include/config",
         "-I../../third_party/skia/include/core",
         "-I../../third_party/skia/include/effects",
         "-I../../third_party/skia/include/encode",
         "-I../../third_party/skia/include/gpu",
         "-I../../third_party/skia/include/images",
         "-I../../third_party/skia/include/lazy",
         "-I../../third_party/skia/include/pathops",
         "-I../../third_party/skia/include/pdf",
         "-I../../third_party/skia/include/pipe",
         "-I../../third_party/skia/include/ports",
         "-I../../third_party/skia/include/utils",
         "-I../../third_party/skia/third_party/vulkan",
         "-I../../third_party/skia/include/codec",
         "-I../../third_party/skia/src/gpu",
         "-I../../third_party/skia/src/sksl",
         "-I../../third_party/ced/src",
         "-I../../third_party/mesa/src/include",
         "-I../../third_party/libwebm/source",
         "-Igen",
         "-I../../build/linux/debian_jessie_amd64-sysroot/"
         "usr/include/dbus-1.0",
         "-I../../build/linux/debian_jessie_amd64-sysroot/"
         "usr/lib/x86_64-linux-gnu/dbus-1.0/include",
         "-I../../third_party/googletest/custom",
         "-I../../third_party/googletest/src/googlemock/"
         "include",
         "-fno-strict-aliasing",
         "-Wno-builtin-macro-redefined",
         "-D__DATE__=",
         "-D__TIME__=",
         "-D__TIMESTAMP__=",
         "-funwind-tables",
         "-fPIC",
         "-pipe",
         "-B../../third_party/binutils/Linux_x64/Release/bin",
         "-pthread",
         "-fcolor-diagnostics",
         "-no-canonical-prefixes",
         "-m64",
         "-march=x86-64",
         "-Wall",
         "-Werror",
         "-Wextra",
         "-Wno-missing-field-initializers",
         "-Wno-unused-parameter",
         "-Wno-c++11-narrowing",
         "-Wno-covered-switch-default",
         "-Wno-unneeded-internal-declaration",
         "-Wno-inconsistent-missing-override",
         "-Wno-undefined-var-template",
         "-Wno-nonportable-include-path",
         "-Wno-address-of-packed-member",
         "-Wno-unused-lambda-capture",
         "-Wno-user-defined-warnings",
         "-Wno-enum-compare-switch",
         "-Wno-tautological-unsigned-zero-compare",
         "-Wno-null-pointer-arithmetic",
         "-Wno-tautological-unsigned-enum-zero-compare",
         "-O2",
         "-fno-ident",
         "-fdata-sections",
         "-ffunction-sections",
         "-fno-omit-frame-pointer",
         "-g0",
         "-fvisibility=hidden",
         "-Wheader-hygiene",
         "-Wstring-conversion",
         "-Wtautological-overlap-compare",
         "-Wno-header-guard",
         "-std=gnu++14",
         "-fno-rtti",
         "-nostdinc++",
         "-isystem../../buildtools/third_party/libc++/"
         "trunk/"
         "include",
         "-isystem../../buildtools/third_party/libc++abi/"
         "trunk/"
         "include",
         "--sysroot=&/w/c/s/out/Release/../../build/linux/"
         "debian_jessie_amd64-sysroot",
         "-fno-exceptions",
         "-fvisibility-inlines-hidden",
         "&/w/c/s/out/Release/../../ash/login/ui/"
         "lock_screen_sanity_unittest.cc",
         "-resource-dir=/w/resource_dir/",
         "-Wno-unknown-warning-option",
         "-fparse-all-comments"});
  }

  // Checks flag parsing for an example chromium file.
  TEST_CASE("chromium") {
    CheckFlags(
        "/w/c/s/out/Release", "../../apps/app_lifetime_monitor.cc",
        /* raw */
        {"/work/goma/gomacc",
         "../../third_party/llvm-build/Release+Asserts/bin/clang++",
         "-MMD",
         "-MF",
         "obj/apps/apps/app_lifetime_monitor.o.d",
         "-DV8_DEPRECATION_WARNINGS",
         "-DDCHECK_ALWAYS_ON=1",
         "-DUSE_UDEV",
         "-DUSE_ASH=1",
         "-DUSE_AURA=1",
         "-DUSE_NSS_CERTS=1",
         "-DUSE_OZONE=1",
         "-DDISABLE_NACL",
         "-DFULL_SAFE_BROWSING",
         "-DSAFE_BROWSING_CSD",
         "-DSAFE_BROWSING_DB_LOCAL",
         "-DCHROMIUM_BUILD",
         "-DFIELDTRIAL_TESTING_ENABLED",
         "-DCR_CLANG_REVISION=\"310694-1\"",
         "-D_FILE_OFFSET_BITS=64",
         "-D_LARGEFILE_SOURCE",
         "-D_LARGEFILE64_SOURCE",
         "-D__STDC_CONSTANT_MACROS",
         "-D__STDC_FORMAT_MACROS",
         "-DCOMPONENT_BUILD",
         "-DOS_CHROMEOS",
         "-DNDEBUG",
         "-DNVALGRIND",
         "-DDYNAMIC_ANNOTATIONS_ENABLED=0",
         "-DGL_GLEXT_PROTOTYPES",
         "-DUSE_GLX",
         "-DUSE_EGL",
         "-DANGLE_ENABLE_RELEASE_ASSERTS",
         "-DTOOLKIT_VIEWS=1",
         "-DV8_USE_EXTERNAL_STARTUP_DATA",
         "-DU_USING_ICU_NAMESPACE=0",
         "-DU_ENABLE_DYLOAD=0",
         "-DICU_UTIL_DATA_IMPL=ICU_UTIL_DATA_FILE",
         "-DUCHAR_TYPE=uint16_t",
         "-DGOOGLE_PROTOBUF_NO_RTTI",
         "-DGOOGLE_PROTOBUF_NO_STATIC_INITIALIZER",
         "-DHAVE_PTHREAD",
         "-DPROTOBUF_USE_DLLS",
         "-DSK_IGNORE_LINEONLY_AA_CONVEX_PATH_OPTS",
         "-DSK_HAS_PNG_LIBRARY",
         "-DSK_HAS_WEBP_LIBRARY",
         "-DSK_HAS_JPEG_LIBRARY",
         "-DSKIA_DLL",
         "-DGR_GL_IGNORE_ES3_MSAA=0",
         "-DSK_SUPPORT_GPU=1",
         "-DMESA_EGL_NO_X11_HEADERS",
         "-DBORINGSSL_SHARED_LIBRARY",
         "-DUSING_V8_SHARED",
         "-I../..",
         "-Igen",
         "-I../../third_party/libwebp/src",
         "-I../../third_party/khronos",
         "-I../../gpu",
         "-I../../third_party/ced/src",
         "-I../../third_party/icu/source/common",
         "-I../../third_party/icu/source/i18n",
         "-I../../third_party/protobuf/src",
         "-I../../skia/config",
         "-I../../skia/ext",
         "-I../../third_party/skia/include/c",
         "-I../../third_party/skia/include/config",
         "-I../../third_party/skia/include/core",
         "-I../../third_party/skia/include/effects",
         "-I../../third_party/skia/include/encode",
         "-I../../third_party/skia/include/gpu",
         "-I../../third_party/skia/include/images",
         "-I../../third_party/skia/include/lazy",
         "-I../../third_party/skia/include/pathops",
         "-I../../third_party/skia/include/pdf",
         "-I../../third_party/skia/include/pipe",
         "-I../../third_party/skia/include/ports",
         "-I../../third_party/skia/include/utils",
         "-I../../third_party/skia/third_party/vulkan",
         "-I../../third_party/skia/src/gpu",
         "-I../../third_party/skia/src/sksl",
         "-I../../third_party/mesa/src/include",
         "-I../../third_party/libwebm/source",
         "-I../../third_party/protobuf/src",
         "-Igen/protoc_out",
         "-I../../third_party/boringssl/src/include",
         "-I../../build/linux/debian_jessie_amd64-sysroot/usr/include/nss",
         "-I../../build/linux/debian_jessie_amd64-sysroot/usr/include/nspr",
         "-Igen",
         "-I../../third_party/WebKit",
         "-Igen/third_party/WebKit",
         "-I../../v8/include",
         "-Igen/v8/include",
         "-Igen",
         "-I../../third_party/flatbuffers/src/include",
         "-Igen",
         "-fno-strict-aliasing",
         "-Wno-builtin-macro-redefined",
         "-D__DATE__=",
         "-D__TIME__=",
         "-D__TIMESTAMP__=",
         "-funwind-tables",
         "-fPIC",
         "-pipe",
         "-B../../third_party/binutils/Linux_x64/Release/bin",
         "-pthread",
         "-fcolor-diagnostics",
         "-m64",
         "-march=x86-64",
         "-Wall",
         "-Werror",
         "-Wextra",
         "-Wno-missing-field-initializers",
         "-Wno-unused-parameter",
         "-Wno-c++11-narrowing",
         "-Wno-covered-switch-default",
         "-Wno-unneeded-internal-declaration",
         "-Wno-inconsistent-missing-override",
         "-Wno-undefined-var-template",
         "-Wno-nonportable-include-path",
         "-Wno-address-of-packed-member",
         "-Wno-unused-lambda-capture",
         "-Wno-user-defined-warnings",
         "-Wno-enum-compare-switch",
         "-O2",
         "-fno-ident",
         "-fdata-sections",
         "-ffunction-sections",
         "-fno-omit-frame-pointer",
         "-g0",
         "-fvisibility=hidden",
         "-Xclang",
         "-load",
         "-Xclang",
         "../../third_party/llvm-build/Release+Asserts/lib/"
         "libFindBadConstructs.so",
         "-Xclang",
         "-add-plugin",
         "-Xclang",
         "find-bad-constructs",
         "-Xclang",
         "-plugin-arg-find-bad-constructs",
         "-Xclang",
         "check-auto-raw-pointer",
         "-Xclang",
         "-plugin-arg-find-bad-constructs",
         "-Xclang",
         "check-ipc",
         "-Wheader-hygiene",
         "-Wstring-conversion",
         "-Wtautological-overlap-compare",
         "-Wexit-time-destructors",
         "-Wno-header-guard",
         "-Wno-exit-time-destructors",
         "-std=gnu++14",
         "-fno-rtti",
         "-nostdinc++",
         "-isystem../../buildtools/third_party/libc++/trunk/include",
         "-isystem../../buildtools/third_party/libc++abi/trunk/include",
         "--sysroot=../../build/linux/debian_jessie_amd64-sysroot",
         "-fno-exceptions",
         "-fvisibility-inlines-hidden",
         "../../apps/app_lifetime_monitor.cc"},

        /* expected */
        {"../../third_party/llvm-build/Release+Asserts/bin/clang++",
         "-working-directory=/w/c/s/out/Release",
         "-DV8_DEPRECATION_WARNINGS",
         "-DDCHECK_ALWAYS_ON=1",
         "-DUSE_UDEV",
         "-DUSE_ASH=1",
         "-DUSE_AURA=1",
         "-DUSE_NSS_CERTS=1",
         "-DUSE_OZONE=1",
         "-DDISABLE_NACL",
         "-DFULL_SAFE_BROWSING",
         "-DSAFE_BROWSING_CSD",
         "-DSAFE_BROWSING_DB_LOCAL",
         "-DCHROMIUM_BUILD",
         "-DFIELDTRIAL_TESTING_ENABLED",
         "-DCR_CLANG_REVISION=\"310694-1\"",
         "-D_FILE_OFFSET_BITS=64",
         "-D_LARGEFILE_SOURCE",
         "-D_LARGEFILE64_SOURCE",
         "-D__STDC_CONSTANT_MACROS",
         "-D__STDC_FORMAT_MACROS",
         "-DCOMPONENT_BUILD",
         "-DOS_CHROMEOS",
         "-DNDEBUG",
         "-DNVALGRIND",
         "-DDYNAMIC_ANNOTATIONS_ENABLED=0",
         "-DGL_GLEXT_PROTOTYPES",
         "-DUSE_GLX",
         "-DUSE_EGL",
         "-DANGLE_ENABLE_RELEASE_ASSERTS",
         "-DTOOLKIT_VIEWS=1",
         "-DV8_USE_EXTERNAL_STARTUP_DATA",
         "-DU_USING_ICU_NAMESPACE=0",
         "-DU_ENABLE_DYLOAD=0",
         "-DICU_UTIL_DATA_IMPL=ICU_UTIL_DATA_FILE",
         "-DUCHAR_TYPE=uint16_t",
         "-DGOOGLE_PROTOBUF_NO_RTTI",
         "-DGOOGLE_PROTOBUF_NO_STATIC_INITIALIZER",
         "-DHAVE_PTHREAD",
         "-DPROTOBUF_USE_DLLS",
         "-DSK_IGNORE_LINEONLY_AA_CONVEX_PATH_OPTS",
         "-DSK_HAS_PNG_LIBRARY",
         "-DSK_HAS_WEBP_LIBRARY",
         "-DSK_HAS_JPEG_LIBRARY",
         "-DSKIA_DLL",
         "-DGR_GL_IGNORE_ES3_MSAA=0",
         "-DSK_SUPPORT_GPU=1",
         "-DMESA_EGL_NO_X11_HEADERS",
         "-DBORINGSSL_SHARED_LIBRARY",
         "-DUSING_V8_SHARED",
         "-I../..",
         "-Igen",
         "-I../../third_party/libwebp/src",
         "-I../../third_party/khronos",
         "-I../../gpu",
         "-I../../third_party/ced/src",
         "-I../../third_party/icu/source/common",
         "-I../../third_party/icu/source/i18n",
         "-I../../third_party/protobuf/src",
         "-I../../skia/config",
         "-I../../skia/ext",
         "-I../../third_party/skia/include/c",
         "-I../../third_party/skia/include/config",
         "-I../../third_party/skia/include/core",
         "-I../../third_party/skia/include/effects",
         "-I../../third_party/skia/include/encode",
         "-I../../third_party/skia/include/gpu",
         "-I../../third_party/skia/include/images",
         "-I../../third_party/skia/include/lazy",
         "-I../../third_party/skia/include/pathops",
         "-I../../third_party/skia/include/pdf",
         "-I../../third_party/skia/include/pipe",
         "-I../../third_party/skia/include/ports",
         "-I../../third_party/skia/include/utils",
         "-I../../third_party/skia/third_party/vulkan",
         "-I../../third_party/skia/src/gpu",
         "-I../../third_party/skia/src/sksl",
         "-I../../third_party/mesa/src/include",
         "-I../../third_party/libwebm/source",
         "-I../../third_party/protobuf/src",
         "-Igen/protoc_out",
         "-I../../third_party/boringssl/src/include",
         "-I../../build/linux/debian_jessie_amd64-sysroot/"
         "usr/include/nss",
         "-I../../build/linux/debian_jessie_amd64-sysroot/"
         "usr/include/nspr",
         "-Igen",
         "-I../../third_party/WebKit",
         "-Igen/third_party/WebKit",
         "-I../../v8/include",
         "-Igen/v8/include",
         "-Igen",
         "-I../../third_party/flatbuffers/src/include",
         "-Igen",
         "-fno-strict-aliasing",
         "-Wno-builtin-macro-redefined",
         "-D__DATE__=",
         "-D__TIME__=",
         "-D__TIMESTAMP__=",
         "-funwind-tables",
         "-fPIC",
         "-pipe",
         "-B../../third_party/binutils/Linux_x64/Release/bin",
         "-pthread",
         "-fcolor-diagnostics",
         "-m64",
         "-march=x86-64",
         "-Wall",
         "-Werror",
         "-Wextra",
         "-Wno-missing-field-initializers",
         "-Wno-unused-parameter",
         "-Wno-c++11-narrowing",
         "-Wno-covered-switch-default",
         "-Wno-unneeded-internal-declaration",
         "-Wno-inconsistent-missing-override",
         "-Wno-undefined-var-template",
         "-Wno-nonportable-include-path",
         "-Wno-address-of-packed-member",
         "-Wno-unused-lambda-capture",
         "-Wno-user-defined-warnings",
         "-Wno-enum-compare-switch",
         "-O2",
         "-fno-ident",
         "-fdata-sections",
         "-ffunction-sections",
         "-fno-omit-frame-pointer",
         "-g0",
         "-fvisibility=hidden",
         "-Wheader-hygiene",
         "-Wstring-conversion",
         "-Wtautological-overlap-compare",
         "-Wexit-time-destructors",
         "-Wno-header-guard",
         "-Wno-exit-time-destructors",
         "-std=gnu++14",
         "-fno-rtti",
         "-nostdinc++",
         "-isystem../../buildtools/third_party/libc++/"
         "trunk/"
         "include",
         "-isystem../../buildtools/third_party/libc++abi/"
         "trunk/"
         "include",
         "--sysroot=&/w/c/s/out/Release/../../build/linux/"
         "debian_jessie_amd64-sysroot",
         "-fno-exceptions",
         "-fvisibility-inlines-hidden",
         "&/w/c/s/out/Release/../../apps/app_lifetime_monitor.cc",
         "-resource-dir=/w/resource_dir/",
         "-Wno-unknown-warning-option",
         "-fparse-all-comments"});
  }

  TEST_CASE("Directory extraction") {
    Config init_opts;
    ProjectConfig config;
    config.project_dir = "/w/c/s/";

    CompileCommandsEntry entry;
    entry.directory = "/base";
    entry.args = {"clang",
                  "-I/a_absolute1",
                  "--foobar",
                  "-I",
                  "/a_absolute2",
                  "--foobar",
                  "-Ia_relative1",
                  "--foobar",
                  "-I",
                  "a_relative2",
                  "--foobar",
                  "-iquote/q_absolute1",
                  "--foobar",
                  "-iquote",
                  "/q_absolute2",
                  "--foobar",
                  "-iquoteq_relative1",
                  "--foobar",
                  "-iquote",
                  "q_relative2",
                  "--foobar",
                  "foo.cc"};
    entry.file = "foo.cc";
    Project::Entry result =
        GetCompilationEntryFromCompileCommandEntry(&init_opts, &config, entry);

    std::unordered_set<std::string> angle_expected{
        "&/a_absolute1", "&/a_absolute2", "&/base/a_relative1",
        "&/base/a_relative2"};
    std::unordered_set<std::string> quote_expected{
        "&/q_absolute1", "&/q_absolute2", "&/base/q_relative1",
        "&/base/q_relative2"};
    REQUIRE(config.angle_dirs == angle_expected);
    REQUIRE(config.quote_dirs == quote_expected);
  }

  TEST_CASE("Entry inference") {
    Project p;
    {
      Project::Entry e;
      e.args = {"arg1"};
      e.filename = "/a/b/c/d/bar.cc";
      p.entries.push_back(e);
    }
    {
      Project::Entry e;
      e.args = {"arg2"};
      e.filename = "/a/b/c/baz.cc";
      p.entries.push_back(e);
    }

    // Guess at same directory level, when there are parent directories.
    {
      optional<Project::Entry> entry =
          p.FindCompilationEntryForFile("/a/b/c/d/new.cc");
      REQUIRE(entry.has_value());
      REQUIRE(entry->args == std::vector<std::string>{"arg1"});
    }

    // Guess at same directory level, when there are child directories.
    {
      optional<Project::Entry> entry =
          p.FindCompilationEntryForFile("/a/b/c/new.cc");
      REQUIRE(entry.has_value());
      REQUIRE(entry->args == std::vector<std::string>{"arg2"});
    }

    // Guess at new directory (use the closest parent directory).
    {
      optional<Project::Entry> entry =
          p.FindCompilationEntryForFile("/a/b/c/new/new.cc");
      REQUIRE(entry.has_value());
      REQUIRE(entry->args == std::vector<std::string>{"arg2"});
    }
  }

  TEST_CASE("Entry inference remaps file names") {
    Project p;
    {
      Project::Entry e;
      e.args = {"a", "b", "aaaa.cc", "d"};
      e.filename = "absolute/aaaa.cc";
      p.entries.push_back(e);
    }

    {
      optional<Project::Entry> entry = p.FindCompilationEntryForFile("ee.cc");
      REQUIRE(entry.has_value());
      REQUIRE(entry->args == std::vector<std::string>{"a", "b", "ee.cc", "d"});
    }
  }

  TEST_CASE("IsWindowsAbsolutePath works correctly") {
    REQUIRE(IsWindowsAbsolutePath("C:/Users/projects/"));
    REQUIRE(IsWindowsAbsolutePath("C:/Users/projects"));
    REQUIRE(IsWindowsAbsolutePath("C:/Users/projects"));
    REQUIRE(IsWindowsAbsolutePath("C:\\Users\\projects"));
    REQUIRE(IsWindowsAbsolutePath("C:\\\\Users\\\\projects"));
    REQUIRE(IsWindowsAbsolutePath("c:\\\\Users\\\\projects"));
    REQUIRE(IsWindowsAbsolutePath("A:\\\\Users\\\\projects"));

    REQUIRE(!IsWindowsAbsolutePath("C:/"));
    REQUIRE(!IsWindowsAbsolutePath("../abc/test"));
    REQUIRE(!IsWindowsAbsolutePath("5:/test"));
    REQUIRE(!IsWindowsAbsolutePath("cquery/project/file.cc"));
    REQUIRE(!IsWindowsAbsolutePath(""));
    REQUIRE(!IsWindowsAbsolutePath("/etc/linux/path"));
  }

  TEST_CASE("Entry inference prefers same file endings") {
    Project p;
    {
      Project::Entry e;
      e.args = {"arg1"};
      e.filename = "common/simple_browsertest.cc";
      p.entries.push_back(e);
    }
    {
      Project::Entry e;
      e.args = {"arg2"};
      e.filename = "common/simple_unittest.cc";
      p.entries.push_back(e);
    }
    {
      Project::Entry e;
      e.args = {"arg3"};
      e.filename = "common/a/simple_unittest.cc";
      p.entries.push_back(e);
    }

    // Prefer files with the same ending.
    {
      optional<Project::Entry> entry =
          p.FindCompilationEntryForFile("my_browsertest.cc");
      REQUIRE(entry.has_value());
      REQUIRE(entry->args == std::vector<std::string>{"arg1"});
    }
    {
      optional<Project::Entry> entry =
          p.FindCompilationEntryForFile("my_unittest.cc");
      REQUIRE(entry.has_value());
      REQUIRE(entry->args == std::vector<std::string>{"arg2"});
    }
    {
      optional<Project::Entry> entry =
          p.FindCompilationEntryForFile("common/my_browsertest.cc");
      REQUIRE(entry.has_value());
      REQUIRE(entry->args == std::vector<std::string>{"arg1"});
    }
    {
      optional<Project::Entry> entry =
          p.FindCompilationEntryForFile("common/my_unittest.cc");
      REQUIRE(entry.has_value());
      REQUIRE(entry->args == std::vector<std::string>{"arg2"});
    }

    // Prefer the same directory over matching file-ending.
    {
      optional<Project::Entry> entry =
          p.FindCompilationEntryForFile("common/a/foo.cc");
      REQUIRE(entry.has_value());
      REQUIRE(entry->args == std::vector<std::string>{"arg3"});
    }
  }
}
