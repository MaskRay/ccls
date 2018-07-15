#include "project.h"

#include "clang_utils.h"
#include "filesystem.hh"
#include "language.h"
#include "log.hh"
#include "match.h"
#include "platform.h"
#include "pipeline.hh"
#include "serializers/json.h"
#include "utils.h"
#include "working_files.h"
using namespace ccls;

#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/LineIterator.h>
using namespace clang;
using namespace llvm;
using namespace llvm::opt;

#include <rapidjson/writer.h>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

#include <limits>
#include <unordered_set>
#include <vector>

struct CompileCommandsEntry {
  std::string directory;
  std::string file;
  std::string command;
  std::vector<std::string> args;

  std::string ResolveIfRelative(std::string path) const {
    if (sys::path::is_absolute(path))
      return path;
    SmallString<256> Ret;
    sys::path::append(Ret, directory, path);
    return NormalizePath(Ret.str());
  }
};
MAKE_REFLECT_STRUCT(CompileCommandsEntry, directory, file, command, args);

namespace {

enum class ProjectMode { CompileCommandsJson, DotCcls, ExternalCommand };

struct ProjectConfig {
  std::unordered_set<std::string> quote_dirs;
  std::unordered_set<std::string> angle_dirs;
  std::vector<std::string> extra_flags;
  std::string project_dir;
  ProjectMode mode = ProjectMode::CompileCommandsJson;
};

enum OptionClass {
  EqOrJoinOrSep,
  EqOrSep,
  JoinOrSep,
  Separate,
};

Project::Entry GetCompilationEntryFromCompileCommandEntry(
    ProjectConfig* config,
    const CompileCommandsEntry& entry) {
  Project::Entry result;
  result.filename = entry.file;
  const std::string base_name = sys::path::filename(entry.file);

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
  args.insert(args.end(), config->extra_flags.begin(),
              config->extra_flags.end());

  // a weird C++ deduction guide heap-use-after-free causes libclang to crash.
  IgnoringDiagConsumer DiagC;
  IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts(new DiagnosticOptions());
  DiagnosticsEngine Diags(
      IntrusiveRefCntPtr<DiagnosticIDs>(new DiagnosticIDs()), &*DiagOpts,
      &DiagC, false);

  driver::Driver Driver(args[0], llvm::sys::getDefaultTargetTriple(), Diags);
  auto TargetAndMode =
      driver::ToolChain::getTargetAndModeFromProgramName(args[0]);
  if (!TargetAndMode.TargetPrefix.empty()) {
    const char* arr[] = {"-target", TargetAndMode.TargetPrefix.c_str()};
    args.insert(args.begin() + 1, std::begin(arr), std::end(arr));
    Driver.setTargetAndMode(TargetAndMode);
  }
  Driver.setCheckInputsExist(false);

  std::vector<const char*> cargs;
  for (auto& arg : args)
    cargs.push_back(arg.c_str());
  cargs.push_back("-fsyntax-only");
  std::unique_ptr<driver::Compilation> C(Driver.BuildCompilation(cargs));
  const driver::JobList& Jobs = C->getJobs();
  if (Jobs.size() != 1)
    return result;
  const driver::ArgStringList& CCArgs = Jobs.begin()->getArguments();

  auto CI = std::make_unique<CompilerInvocation>();
  CompilerInvocation::CreateFromArgs(*CI, CCArgs.data(),
                                     CCArgs.data() + CCArgs.size(), Diags);
  CI->getFrontendOpts().DisableFree = false;
  CI->getCodeGenOpts().DisableFree = false;

  HeaderSearchOptions &HeaderOpts = CI->getHeaderSearchOpts();
  for (auto &E : HeaderOpts.UserEntries) {
    std::string path = entry.ResolveIfRelative(E.Path);
    switch (E.Group) {
      default:
        config->angle_dirs.insert(path);
        break;
      case frontend::Quoted:
        config->quote_dirs.insert(path);
        break;
      case frontend::Angled:
        config->angle_dirs.insert(path);
        config->quote_dirs.insert(path);
        break;
    }
  }

  for (size_t i = 1; i < args.size(); i++)
    // This is most likely the file path we will be passing to clang. The
    // path needs to be absolute, otherwise clang_codeCompleteAt is extremely
    // slow. See
    // https://github.com/cquery-project/cquery/commit/af63df09d57d765ce12d40007bf56302a0446678.
    if (args[i][0] != '-' && EndsWith(args[i], base_name)) {
      args[i] = entry.ResolveIfRelative(args[i]);
      continue;
    }

  if (HeaderOpts.ResourceDir.empty() && HeaderOpts.UseBuiltinIncludes)
    args.push_back("-resource-dir=" + g_config->clang.resourceDir);
  if (CI->getFileSystemOpts().WorkingDir.empty())
    args.push_back("-working-directory=" + entry.directory);

  // There could be a clang version mismatch between what the project uses and
  // what ccls uses. Make sure we do not emit warnings for mismatched options.
  args.push_back("-Wno-unknown-warning-option");

  result.directory = entry.directory;
  result.args = std::move(args);
  return result;
}

std::vector<std::string> ReadCompilerArgumentsFromFile(
    const std::string& path) {
  auto MBOrErr = MemoryBuffer::getFile(path);
  if (!MBOrErr) return {};
  std::vector<std::string> args;
  for (line_iterator I(*MBOrErr.get(), true, '#'), E; I != E; ++I)
    args.push_back(*I);
  return args;
}

std::vector<Project::Entry> LoadFromDirectoryListing(ProjectConfig* config) {
  std::vector<Project::Entry> result;
  config->mode = ProjectMode::DotCcls;
  SmallString<256> Path;
  sys::path::append(Path, config->project_dir, ".ccls");
  LOG_IF_S(WARNING, !sys::fs::exists(Path) && config->extra_flags.empty())
      << "ccls has no clang arguments. Use either "
         "compile_commands.json or .ccls, See ccls README for "
         "more information.";

  std::unordered_map<std::string, std::vector<std::string>> folder_args;
  std::vector<std::string> files;

  GetFilesInFolder(config->project_dir, true /*recursive*/,
                   true /*add_folder_to_path*/,
                   [&folder_args, &files](const std::string& path) {
                     if (SourceFileLanguage(path) != LanguageId::Unknown) {
                       files.push_back(path);
                     } else if (sys::path::filename(path) == ".ccls") {
                       LOG_S(INFO) << "Using .ccls arguments from " << path;
                       folder_args.emplace(sys::path::parent_path(path),
                                           ReadCompilerArgumentsFromFile(path));
                     }
                   });

  const std::string& project_dir = config->project_dir;
  const auto& project_dir_args = folder_args[project_dir];
  LOG_IF_S(INFO, !project_dir_args.empty())
      << "Using .ccls arguments " << StringJoin(project_dir_args);

  auto GetCompilerArgumentForFile = [&project_dir, &folder_args](std::string cur) {
    while (!(cur = sys::path::parent_path(cur)).empty()) {
      auto it = folder_args.find(cur);
      if (it != folder_args.end())
        return it->second;
      std::string normalized = NormalizePath(cur);
      // Break if outside of the project root.
      if (normalized.size() <= project_dir.size() ||
          normalized.compare(0, project_dir.size(), project_dir) != 0)
        break;
    }
    return folder_args[project_dir];
  };

  for (const std::string& file : files) {
    CompileCommandsEntry e;
    e.directory = config->project_dir;
    e.file = file;
    e.args = GetCompilerArgumentForFile(file);
    if (e.args.empty())
      e.args.push_back("%clang");  // Add a Dummy.
    e.args.push_back(e.file);
    result.push_back(GetCompilationEntryFromCompileCommandEntry(config, e));
  }

  return result;
}

std::vector<Project::Entry> LoadCompilationEntriesFromDirectory(
    ProjectConfig* project,
    const std::string& opt_compilation_db_dir) {
  // If there is a .ccls file always load using directory listing.
  SmallString<256> Path;
  sys::path::append(Path, project->project_dir, ".ccls");
  if (sys::fs::exists(Path))
    return LoadFromDirectoryListing(project);

  // If |compilationDatabaseCommand| is specified, execute it to get the compdb.
  std::string comp_db_dir;
  Path.clear();
  if (g_config->compilationDatabaseCommand.empty()) {
    project->mode = ProjectMode::CompileCommandsJson;
    // Try to load compile_commands.json, but fallback to a project listing.
    comp_db_dir = opt_compilation_db_dir.empty() ? project->project_dir
                                                 : opt_compilation_db_dir;
    sys::path::append(Path, comp_db_dir, "compile_commands.json");
  } else {
    project->mode = ProjectMode::ExternalCommand;
#ifdef _WIN32
    // TODO
#else
    char tmpdir[] = "/tmp/ccls-compdb-XXXXXX";
    if (!mkdtemp(tmpdir))
      return {};
    comp_db_dir = tmpdir;
    sys::path::append(Path, comp_db_dir, "compile_commands.json");
    rapidjson::StringBuffer input;
    rapidjson::Writer<rapidjson::StringBuffer> writer(input);
    JsonWriter json_writer(&writer);
    Reflect(json_writer, *g_config);
    std::string contents = GetExternalCommandOutput(
        std::vector<std::string>{g_config->compilationDatabaseCommand,
                                 project->project_dir},
        input.GetString());
    FILE* fout = fopen(Path.c_str(), "wb");
    fwrite(contents.c_str(), contents.size(), 1, fout);
    fclose(fout);
#endif
  }

  std::string err_msg;
  std::unique_ptr<tooling::CompilationDatabase> CDB =
      tooling::CompilationDatabase::loadFromDirectory(comp_db_dir, err_msg);
  if (!g_config->compilationDatabaseCommand.empty()) {
#ifdef _WIN32
  // TODO
#else
    unlink(Path.c_str());
    rmdir(comp_db_dir.c_str());
#endif
  }
  if (!CDB) {
    LOG_S(WARNING) << "failed to load " << Path.c_str() << " " << err_msg;
    return {};
  }

  LOG_S(INFO) << "loaded " << Path.c_str();

  std::vector<Project::Entry> result;
  for (tooling::CompileCommand &Cmd : CDB->getAllCompileCommands()) {
    CompileCommandsEntry entry;
    entry.directory = std::move(Cmd.Directory);
    entry.file = entry.ResolveIfRelative(Cmd.Filename);
    entry.args = std::move(Cmd.CommandLine);
    result.push_back(
        GetCompilationEntryFromCompileCommandEntry(project, entry));
  }
  return result;
}

// Computes a score based on how well |a| and |b| match. This is used for
// argument guessing.
int ComputeGuessScore(std::string_view a, std::string_view b) {
  // Increase score based on common prefix and suffix. Prefixes are prioritized.
  if (a.size() < b.size())
    std::swap(a, b);
  size_t i = std::mismatch(a.begin(), a.end(), b.begin()).first - a.begin();
  size_t j = std::mismatch(a.rbegin(), a.rend(), b.rbegin()).first - a.rbegin();
  int score = 10 * i + j;
  if (i + j < b.size())
    score -= 100 * (std::count(a.begin() + i, a.end() - j, '/') +
                    std::count(b.begin() + i, b.end() - j, '/'));
  return score;
}

}  // namespace

bool Project::loaded = false;

void Project::Load(const std::string& root_directory) {
  Project::loaded = false;
  // Load data.
  ProjectConfig project;
  project.extra_flags = g_config->clang.extraArgs;
  project.project_dir = root_directory;
  entries = LoadCompilationEntriesFromDirectory(
      &project, g_config->compilationDatabaseDirectory);

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
  std::lock_guard<std::mutex> lock(mutex_);
  absolute_path_to_entry_index_.reserve(entries.size());
  for (size_t i = 0; i < entries.size(); ++i) {
    entries[i].id = i;
    absolute_path_to_entry_index_[entries[i].filename] = i;
  }
}

void Project::SetFlagsForFile(
    const std::vector<std::string>& flags,
    const std::string& path) {
  std::lock_guard<std::mutex> lock(mutex_);
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
  {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = absolute_path_to_entry_index_.find(filename);
    if (it != absolute_path_to_entry_index_.end())
      return entries[it->second];
  }

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
    std::string best_entry_base_name = sys::path::filename(best_entry->filename);
    for (std::string& arg : result.args) {
      try {
        if (arg == best_entry->filename ||
            sys::path::filename(arg) == best_entry_base_name)
          arg = filename;
      } catch (...) {
      }
    }
  }

  return result;
}

void Project::ForAllFilteredFiles(
    std::function<void(int i, const Entry& entry)> action) {
  GroupMatch matcher(g_config->index.whitelist, g_config->index.blacklist);
  for (int i = 0; i < entries.size(); ++i) {
    const Project::Entry& entry = entries[i];
    std::string failure_reason;
    if (matcher.IsMatch(entry.filename, &failure_reason))
      action(i, entries[i]);
    else {
      LOG_V(1) << "[" << i + 1 << "/" << entries.size() << "]: Failed "
               << failure_reason << "; skipping " << entry.filename;
    }
  }
}

void Project::Index(WorkingFiles* wfiles,
                    lsRequestId id) {
  ForAllFilteredFiles([&](int i, const Project::Entry& entry) {
    bool is_interactive = wfiles->GetFileByFilename(entry.filename) != nullptr;
    pipeline::Index(entry.filename, entry.args, is_interactive, id);
  });
  // Dummy request to indicate that project is loaded and
  // trigger refreshing semantic highlight for all working files.
  pipeline::Index("", {}, false);
}
