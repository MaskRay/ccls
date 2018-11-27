/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "project.hh"

#include "clang_tu.hh" // llvm::vfs
#include "filesystem.hh"
#include "log.hh"
#include "pipeline.hh"
#include "platform.hh"
#include "serializers/json.hh"
#include "utils.hh"
#include "working_files.hh"

#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/Types.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/CompilationDatabase.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringSet.h>
#include <llvm/Support/LineIterator.h>

#include <rapidjson/writer.h>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

#include <limits.h>
#include <unordered_set>
#include <vector>

using namespace clang;
using namespace llvm;

namespace ccls {
std::pair<LanguageId, bool> lookupExtension(std::string_view filename) {
  using namespace clang::driver;
  auto I = types::lookupTypeForExtension(
      sys::path::extension({filename.data(), filename.size()}).substr(1));
  bool header = I == types::TY_CHeader || I == types::TY_CXXHeader ||
                I == types::TY_ObjCXXHeader;
  bool objc = types::isObjC(I);
  LanguageId ret;
  if (types::isCXX(I))
    ret = objc ? LanguageId::ObjCpp : LanguageId::Cpp;
  else if (objc)
    ret = LanguageId::ObjC;
  else if (I == types::TY_C || I == types::TY_CHeader)
    ret = LanguageId::C;
  else
    ret = LanguageId::Unknown;
  return {ret, header};
}

namespace {

enum class ProjectMode { CompileCommandsJson, DotCcls, ExternalCommand };

struct ProjectConfig {
  std::unordered_set<std::string> quote_dirs;
  std::unordered_set<std::string> angle_dirs;
  std::string root;
  ProjectMode mode = ProjectMode::CompileCommandsJson;
};

enum OptionClass {
  EqOrJoinOrSep,
  EqOrSep,
  JoinOrSep,
  Separate,
};

struct ProjectProcessor {
  ProjectConfig *config;
  std::unordered_set<size_t> command_set;
  StringSet<> excludeArgs;
  ProjectProcessor(ProjectConfig *config) : config(config) {
    for (auto &arg : g_config->clang.excludeArgs)
      excludeArgs.insert(arg);
  }

  void Process(Project::Entry &entry) {
    const std::string base_name = sys::path::filename(entry.filename);

    // Expand %c %cpp %clang
    std::vector<const char *> args;
    args.reserve(entry.args.size() + g_config->clang.extraArgs.size() + 1);
    const LanguageId lang = lookupExtension(entry.filename).first;
    for (const char *arg : entry.args) {
      StringRef A(arg);
      if (A == "%clang") {
        args.push_back(lang == LanguageId::Cpp || lang == LanguageId::ObjCpp
                           ? "clang++"
                           : "clang");
      } else if (A[0] == '%') {
        bool ok = false;
        for (;;) {
          if (A.consume_front("%c "))
            ok |= lang == LanguageId::C;
          else if (A.consume_front("%cpp "))
            ok |= lang == LanguageId::Cpp;
          else if (A.consume_front("%objective-c "))
            ok |= lang == LanguageId::ObjC;
          else if (A.consume_front("%objective-cpp "))
            ok |= lang == LanguageId::ObjCpp;
          else
            break;
        }
        if (ok)
          args.push_back(A.data());
      } else if (!excludeArgs.count(A)) {
        args.push_back(arg);
      }
    }
    if (args.empty())
      return;
    for (const std::string &arg : g_config->clang.extraArgs)
      args.push_back(Intern(arg));

    size_t hash = std::hash<std::string>{}(entry.directory);
    bool OPT_o = false;
    for (auto &arg : args) {
      bool last_o = OPT_o;
      OPT_o = false;
      if (arg[0] == '-') {
        OPT_o = arg[1] == 'o' && arg[2] == '\0';
        if (OPT_o || arg[1] == 'D' || arg[1] == 'W')
          continue;
      } else if (last_o) {
        continue;
      } else if (sys::path::filename(arg) == base_name) {
        LanguageId lang = lookupExtension(arg).first;
        if (lang != LanguageId::Unknown) {
          hash_combine(hash, (size_t)lang);
          continue;
        }
      }
      hash_combine(hash, std::hash<std::string_view>{}(arg));
    }
    args.push_back(Intern("-working-directory=" + entry.directory));
    entry.args = args;
#if LLVM_VERSION_MAJOR < 8
    args.push_back("-fsyntax-only");
    if (!command_set.insert(hash).second)
      return;

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
      const char *arr[] = {"-target", TargetAndMode.TargetPrefix.c_str()};
      args.insert(args.begin() + 1, std::begin(arr), std::end(arr));
      Driver.setTargetAndMode(TargetAndMode);
    }
    Driver.setCheckInputsExist(false);

    std::unique_ptr<driver::Compilation> C(Driver.BuildCompilation(args));
    const driver::JobList &Jobs = C->getJobs();
    if (Jobs.size() != 1)
      return;
    const auto &CCArgs = Jobs.begin()->getArguments();

    auto CI = std::make_unique<CompilerInvocation>();
    CompilerInvocation::CreateFromArgs(*CI, CCArgs.data(),
      CCArgs.data() + CCArgs.size(), Diags);
    CI->getFrontendOpts().DisableFree = false;
    CI->getCodeGenOpts().DisableFree = false;

    HeaderSearchOptions &HeaderOpts = CI->getHeaderSearchOpts();
    for (auto &E : HeaderOpts.UserEntries) {
      std::string path =
          NormalizePath(ResolveIfRelative(entry.directory, E.Path));
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
#endif
  }
};

std::vector<const char *>
ReadCompilerArgumentsFromFile(const std::string &path) {
  auto MBOrErr = MemoryBuffer::getFile(path);
  if (!MBOrErr)
    return {};
  std::vector<const char *> args;
  for (line_iterator I(*MBOrErr.get(), true, '#'), E; I != E; ++I) {
    std::string line = *I;
    DoPathMapping(line);
    args.push_back(Intern(line));
  }
  return args;
}

std::vector<Project::Entry> LoadFromDirectoryListing(ProjectConfig *config) {
  std::vector<Project::Entry> result;
  config->mode = ProjectMode::DotCcls;

  std::unordered_map<std::string, std::vector<const char *>> folder_args;
  std::vector<std::string> files;
  const std::string &root = config->root;

  GetFilesInFolder(root, true /*recursive*/,
                   true /*add_folder_to_path*/,
                   [&folder_args, &files](const std::string &path) {
                     std::pair<LanguageId, bool> lang = lookupExtension(path);
                     if (lang.first != LanguageId::Unknown && !lang.second) {
                       files.push_back(path);
                     } else if (sys::path::filename(path) == ".ccls") {
                       std::vector<const char *> args = ReadCompilerArgumentsFromFile(path);
                       folder_args.emplace(sys::path::parent_path(path), args);
                       std::string l;
                       for (size_t i = 0; i < args.size(); i++) {
                         if (i)
                           l += ' ';
                         l += args[i];
                       }
                       LOG_S(INFO) << "use " << path << ": " << l;
                     }
                   });

  auto GetCompilerArgumentForFile = [&root,
                                     &folder_args](std::string cur) {
    while (!(cur = sys::path::parent_path(cur)).empty()) {
      auto it = folder_args.find(cur);
      if (it != folder_args.end())
        return it->second;
      std::string normalized = NormalizePath(cur);
      // Break if outside of the project root.
      if (normalized.size() <= root.size() ||
          normalized.compare(0, root.size(), root) != 0)
        break;
    }
    return folder_args[root];
  };

  ProjectProcessor proc(config);
  for (const std::string &file : files) {
    Project::Entry e;
    e.root = config->root;
    e.directory = config->root;
    e.filename = file;
    e.args = GetCompilerArgumentForFile(file);
    if (e.args.empty())
      e.args.push_back("%clang"); // Add a Dummy.
    e.args.push_back(Intern(e.filename));
    proc.Process(e);
    result.push_back(e);
  }

  return result;
}

std::vector<Project::Entry>
LoadEntriesFromDirectory(ProjectConfig *project,
                         const std::string &opt_compdb_dir) {
  // If there is a .ccls file always load using directory listing.
  SmallString<256> Path, CclsPath;
  sys::path::append(CclsPath, project->root, ".ccls");
  if (sys::fs::exists(CclsPath))
    return LoadFromDirectoryListing(project);

  // If |compilationDatabaseCommand| is specified, execute it to get the compdb.
  std::string comp_db_dir;
  if (g_config->compilationDatabaseCommand.empty()) {
    project->mode = ProjectMode::CompileCommandsJson;
    // Try to load compile_commands.json, but fallback to a project listing.
    comp_db_dir = opt_compdb_dir.empty() ? project->root : opt_compdb_dir;
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
                                 project->root},
        input.GetString());
    FILE *fout = fopen(Path.c_str(), "wb");
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
    LOG_S(WARNING) << "no .ccls or compile_commands.json . Consider adding one";
    return LoadFromDirectoryListing(project);
  }

  LOG_S(INFO) << "loaded " << Path.c_str();

  StringSet<> Seen;
  std::vector<Project::Entry> result;
  ProjectProcessor proc(project);
  for (tooling::CompileCommand &Cmd : CDB->getAllCompileCommands()) {
    static bool once;
    Project::Entry entry;
    entry.root = project->root;
    DoPathMapping(entry.root);
    entry.directory = NormalizePath(Cmd.Directory);
    DoPathMapping(entry.directory);
    entry.filename =
        NormalizePath(ResolveIfRelative(entry.directory, Cmd.Filename));
    DoPathMapping(entry.filename);
    std::vector<std::string> args = std::move(Cmd.CommandLine);
    entry.args.reserve(args.size());
    for (std::string &arg : args) {
      DoPathMapping(arg);
      entry.args.push_back(Intern(arg));
    }

    // Work around relative --sysroot= as it isn't affected by
    // -working-directory=. chdir is thread hostile but this function runs
    // before indexers do actual work and it works when there is only one
    // workspace folder.
    if (!once) {
      once = true;
      llvm::vfs::getRealFileSystem()->setCurrentWorkingDirectory(
          entry.directory);
    }
    proc.Process(entry);

    if (Seen.insert(entry.filename).second)
      result.push_back(entry);
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

} // namespace

void Project::Load(const std::string &root) {
  assert(root.back() == '/');
  ProjectConfig project;
  project.root = root;
  Folder &folder = root2folder[root];

  folder.entries = LoadEntriesFromDirectory(
      &project, g_config->compilationDatabaseDirectory);
  folder.quote_search_list.assign(project.quote_dirs.begin(),
                                  project.quote_dirs.end());
  folder.angle_search_list.assign(project.angle_dirs.begin(),
                                  project.angle_dirs.end());
  for (std::string &path : folder.angle_search_list) {
    EnsureEndsInSlash(path);
    LOG_S(INFO) << "angle search: " << path;
  }
  for (std::string &path : folder.quote_search_list) {
    EnsureEndsInSlash(path);
    LOG_S(INFO) << "quote search: " << path;
  }

  // Setup project entries.
  std::lock_guard lock(mutex_);
  folder.path2entry_index.reserve(folder.entries.size());
  for (size_t i = 0; i < folder.entries.size(); ++i) {
    folder.entries[i].id = i;
    folder.path2entry_index[folder.entries[i].filename] = i;
  }
}

void Project::SetArgsForFile(const std::vector<const char *> &args,
                             const std::string &path) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto &[root, folder] : root2folder) {
    auto it = folder.path2entry_index.find(path);
    if (it != folder.path2entry_index.end()) {
      // The entry already exists in the project, just set the flags.
      folder.entries[it->second].args = args;
      return;
    }
  }
}

Project::Entry Project::FindEntry(const std::string &path,
                                  bool can_be_inferred) {
  std::lock_guard<std::mutex> lock(mutex_);
  for (auto &[root, folder] : root2folder) {
    auto it = folder.path2entry_index.find(path);
    if (it != folder.path2entry_index.end()) {
      Project::Entry &entry = folder.entries[it->second];
      if (can_be_inferred || entry.filename == path)
        return entry;
    }
  }

  Project::Entry result;
  const Entry *best_entry = nullptr;
  int best_score = INT_MIN;
  for (auto &[root, folder] : root2folder) {
    for (const Entry &entry : folder.entries) {
      int score = ComputeGuessScore(path, entry.filename);
      if (score > best_score) {
        best_score = score;
        best_entry = &entry;
      }
    }
    if (StringRef(path).startswith(root))
      result.root = root;
  }
  if (result.root.empty())
    result.root = g_config->fallbackFolder;

  result.is_inferred = true;
  result.filename = path;
  if (!best_entry) {
    result.args.push_back("%clang");
    result.args.push_back(Intern(path));
  } else {
    result.args = best_entry->args;

    // |best_entry| probably has its own path in the arguments. We need to remap
    // that path to the new filename.
    std::string best_entry_base_name =
        sys::path::filename(best_entry->filename);
    for (const char *&arg : result.args) {
      try {
        if (arg == best_entry->filename ||
            sys::path::filename(arg) == best_entry_base_name)
          arg = Intern(path);
      } catch (...) {
      }
    }
  }

  return result;
}

void Project::Index(WorkingFiles *wfiles, RequestId id) {
  auto &gi = g_config->index;
  GroupMatch match(gi.whitelist, gi.blacklist),
      match_i(gi.initialWhitelist, gi.initialBlacklist);
  {
    std::lock_guard lock(mutex_);
    for (auto &[root, folder] : root2folder) {
      int i = 0;
      for (const Project::Entry &entry : folder.entries) {
        std::string reason;
        if (match.Matches(entry.filename, &reason) &&
            match_i.Matches(entry.filename, &reason)) {
          bool interactive =
              wfiles->GetFileByFilename(entry.filename) != nullptr;
          pipeline::Index(
              entry.filename, entry.args,
              interactive ? IndexMode::Normal : IndexMode::NonInteractive, id);
        } else {
          LOG_V(1) << "[" << i << "/" << folder.entries.size() << "]: " << reason
                   << "; skip " << entry.filename;
        }
        i++;
      }
    }
  }

  pipeline::loaded_ts = pipeline::tick;
  // Dummy request to indicate that project is loaded and
  // trigger refreshing semantic highlight for all working files.
  pipeline::Index("", {}, IndexMode::NonInteractive);
}
} // namespace ccls
