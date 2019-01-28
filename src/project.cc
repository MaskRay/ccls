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
#include <llvm/Support/Program.h>

#include <rapidjson/writer.h>

#ifdef _WIN32
# include <Windows.h>
#else
# include <unistd.h>
#endif

#include <array>
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

enum OptionClass {
  EqOrJoinOrSep,
  EqOrSep,
  JoinOrSep,
  Separate,
};

struct ProjectProcessor {
  Project::Folder &folder;
  std::unordered_set<size_t> command_set;
  StringSet<> excludeArgs;
  ProjectProcessor(Project::Folder &folder) : folder(folder) {
    for (auto &arg : g_config->clang.excludeArgs)
      excludeArgs.insert(arg);
  }

  // Expand %c %cpp ... in .ccls
  void Process(Project::Entry &entry) {
    std::vector<const char *> args(entry.args.begin(),
                                   entry.args.begin() + entry.compdb_size);
    auto [lang, header] = lookupExtension(entry.filename);
    for (int i = entry.compdb_size; i < entry.args.size(); i++) {
      const char *arg = entry.args[i];
      StringRef A(arg);
      if (A[0] == '%') {
        bool ok = false;
        for (;;) {
          if (A.consume_front("%c "))
            ok |= lang == LanguageId::C;
          else if (A.consume_front("%h "))
            ok |= lang == LanguageId::C && header;
          else if (A.consume_front("%cpp "))
            ok |= lang == LanguageId::Cpp;
          else if (A.consume_front("%hpp "))
            ok |= lang == LanguageId::Cpp && header;
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
    entry.args = args;
    GetSearchDirs(entry);
  }

  void GetSearchDirs(Project::Entry &entry) {
#if LLVM_VERSION_MAJOR < 8
    const std::string base_name = sys::path::filename(entry.filename);
    size_t hash = std::hash<std::string>{}(entry.directory);
    bool OPT_o = false;
    for (auto &arg : entry.args) {
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
    if (!command_set.insert(hash).second)
      return;
    auto args = entry.args;
    args.push_back("-fsyntax-only");
    for (const std::string &arg : g_config->clang.extraArgs)
      args.push_back(Intern(arg));
    args.push_back(Intern("-working-directory=" + entry.directory));
    args.push_back(Intern("-resource-dir=" + g_config->clang.resourceDir));

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
      EnsureEndsInSlash(path);
      switch (E.Group) {
      default:
        folder.search_dir2kind[path] |= 2;
        break;
      case frontend::Quoted:
        folder.search_dir2kind[path] |= 1;
        break;
      case frontend::Angled:
        folder.search_dir2kind[path] |= 3;
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

bool AppendToCDB(const std::vector<const char *> &args) {
  return args.size() && StringRef("%compile_commands.json") == args[0];
}

std::vector<const char *> GetFallback(const std::string path) {
  std::vector<const char *> argv{"clang"};
  if (sys::path::extension(path) == ".h")
    argv.push_back("-xobjective-c++-header");
  argv.push_back(Intern(path));
  return argv;
}

void LoadDirectoryListing(ProjectProcessor &proc, const std::string &root,
                          const StringSet<> &Seen) {
  Project::Folder &folder = proc.folder;
  std::vector<std::string> files;

  auto GetDotCcls = [&root, &folder](std::string cur) {
    while (!(cur = sys::path::parent_path(cur)).empty()) {
      auto it = folder.dot_ccls.find(cur);
      if (it != folder.dot_ccls.end())
        return it->second;
      std::string normalized = NormalizePath(cur);
      // Break if outside of the project root.
      if (normalized.size() <= root.size() ||
          normalized.compare(0, root.size(), root) != 0)
        break;
    }
    return folder.dot_ccls[root];
  };

  GetFilesInFolder(root, true /*recursive*/, true /*add_folder_to_path*/,
                   [&folder, &files, &Seen](const std::string &path) {
                     std::pair<LanguageId, bool> lang = lookupExtension(path);
                     if (lang.first != LanguageId::Unknown && !lang.second) {
                       if (!Seen.count(path))
                         files.push_back(path);
                     } else if (sys::path::filename(path) == ".ccls") {
                       std::vector<const char *> args = ReadCompilerArgumentsFromFile(path);
                       folder.dot_ccls.emplace(sys::path::parent_path(path),
                                               args);
                       std::string l;
                       for (size_t i = 0; i < args.size(); i++) {
                         if (i)
                           l += ' ';
                         l += args[i];
                       }
                       LOG_S(INFO) << "use " << path << ": " << l;
                     }
                   });

  // If the first line of .ccls is %compile_commands.json, append extra flags.
  for (auto &e : folder.entries)
    if (const auto &args = GetDotCcls(e.filename); AppendToCDB(args)) {
      if (args.size())
        e.args.insert(e.args.end(), args.begin() + 1, args.end());
      proc.Process(e);
    }
  // Set flags for files not in compile_commands.json
  for (const std::string &file : files)
    if (const auto &args = GetDotCcls(file); !AppendToCDB(args)) {
      Project::Entry e;
      e.root = e.directory = root;
      e.filename = file;
      if (args.empty()) {
        e.args = GetFallback(e.filename);
      } else {
        e.args = args;
        e.args.push_back(Intern(e.filename));
      }
      proc.Process(e);
      folder.entries.push_back(e);
    }
}

// Computes a score based on how well |a| and |b| match. This is used for
// argument guessing.
int ComputeGuessScore(std::string_view a, std::string_view b) {
  // Increase score based on common prefix and suffix. Prefixes are prioritized.
  if (a.size() > b.size())
    std::swap(a, b);
  size_t i = std::mismatch(a.begin(), a.end(), b.begin()).first - a.begin();
  size_t j = std::mismatch(a.rbegin(), a.rend(), b.rbegin()).first - a.rbegin();
  int score = 10 * i + j;
  if (i + j < a.size())
    score -= 100 * (std::count(a.begin() + i, a.end() - j, '/') +
                    std::count(b.begin() + i, b.end() - j, '/'));
  return score;
}

} // namespace

void Project::LoadDirectory(const std::string &root, Project::Folder &folder) {
  SmallString<256> CDBDir, Path, StdinPath;
  std::string err_msg;
  folder.entries.clear();
  if (g_config->compilationDatabaseCommand.empty()) {
    CDBDir = root;
    if (g_config->compilationDatabaseDirectory.size())
      sys::path::append(CDBDir, g_config->compilationDatabaseDirectory);
    sys::path::append(Path, CDBDir, "compile_commands.json");
  } else {
    // If `compilationDatabaseCommand` is specified, execute it to get the
    // compdb.
#ifdef _WIN32
    char tmpdir[L_tmpnam];
    tmpnam_s(tmpdir, L_tmpnam);
    CDBDir = tmpdir;
    if (sys::fs::create_directory(tmpdir, false))
      return;
#else
    char tmpdir[] = "/tmp/ccls-compdb-XXXXXX";
    if (!mkdtemp(tmpdir))
      return;
    CDBDir = tmpdir;
#endif
    sys::path::append(Path, CDBDir, "compile_commands.json");
    sys::path::append(StdinPath, CDBDir, "stdin");
    {
      rapidjson::StringBuffer sb;
      rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
      JsonWriter json_writer(&writer);
      Reflect(json_writer, *g_config);
      std::string input = sb.GetString();
      FILE *fout = fopen(StdinPath.c_str(), "wb");
      fwrite(input.c_str(), input.size(), 1, fout);
      fclose(fout);
    }
    std::array<Optional<StringRef>, 3> Redir{StringRef(StdinPath),
                                             StringRef(Path), StringRef()};
    std::vector<StringRef> args{g_config->compilationDatabaseCommand, root};
    if (sys::ExecuteAndWait(args[0], args, llvm::None, Redir, 0, 0, &err_msg) <
        0) {
      LOG_S(ERROR) << "failed to execute " << args[0].str() << " "
                   << args[1].str() << ": " << err_msg;
      return;
    }
  }

  std::unique_ptr<tooling::CompilationDatabase> CDB =
      tooling::CompilationDatabase::loadFromDirectory(CDBDir, err_msg);
  if (!g_config->compilationDatabaseCommand.empty()) {
#ifdef _WIN32
    DeleteFileA(StdinPath.c_str());
    DeleteFileA(Path.c_str());
    RemoveDirectoryA(CDBDir.c_str());
#else
    unlink(StdinPath.c_str());
    unlink(Path.c_str());
    rmdir(CDBDir.c_str());
#endif
  }

  ProjectProcessor proc(folder);
  StringSet<> Seen;
  std::vector<Project::Entry> result;
  if (!CDB) {
    if (g_config->compilationDatabaseCommand.size() || sys::fs::exists(Path))
      LOG_S(ERROR) << "failed to load " << Path.c_str();
  } else {
    LOG_S(INFO) << "loaded " << Path.c_str();
    for (tooling::CompileCommand &Cmd : CDB->getAllCompileCommands()) {
      static bool once;
      Project::Entry entry;
      entry.root = root;
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
        if (!proc.excludeArgs.count(arg))
          entry.args.push_back(Intern(arg));
      }
      entry.compdb_size = entry.args.size();

      // Work around relative --sysroot= as it isn't affected by
      // -working-directory=. chdir is thread hostile but this function runs
      // before indexers do actual work and it works when there is only one
      // workspace folder.
      if (!once) {
        once = true;
        llvm::vfs::getRealFileSystem()->setCurrentWorkingDirectory(
            entry.directory);
      }
      proc.GetSearchDirs(entry);

      if (Seen.insert(entry.filename).second)
        folder.entries.push_back(entry);
    }
  }

  // Use directory listing if .ccls exists or compile_commands.json does not
  // exist.
  Path.clear();
  sys::path::append(Path, root, ".ccls");
  if (sys::fs::exists(Path))
    LoadDirectoryListing(proc, root, Seen);
  std::vector<const char *> extra_args;
  for (const std::string &arg : g_config->clang.extraArgs)
    extra_args.push_back(Intern(arg));
  for (auto &e : folder.entries) {
    e.args.insert(e.args.end(), extra_args.begin(), extra_args.end());
    e.args.push_back(Intern("-working-directory=" + e.directory));
  }
}

void Project::Load(const std::string &root) {
  assert(root.back() == '/');
  std::lock_guard lock(mtx);
  Folder &folder = root2folder[root];

  LoadDirectory(root, folder);
  for (auto &[path, kind] : folder.search_dir2kind)
    LOG_S(INFO) << "search directory: " << path << ' ' << " \"< "[kind];

  // Setup project entries.
  folder.path2entry_index.reserve(folder.entries.size());
  for (size_t i = 0; i < folder.entries.size(); ++i) {
    folder.entries[i].id = i;
    folder.path2entry_index[folder.entries[i].filename] = i;
  }
}

Project::Entry Project::FindEntry(const std::string &path, bool can_redirect,
                                  bool must_exist) {
  Project::Folder *best_folder = nullptr;
  const Entry *best = nullptr;
  std::shared_lock lock(mtx);
  for (auto &[root, folder] : root2folder) {
    // The entry may have different filename but it doesn't matter when building
    // CompilerInvocation. The main filename is specified separately.
    auto it = folder.path2entry_index.find(path);
    if (it != folder.path2entry_index.end()) {
      Project::Entry &entry = folder.entries[it->second];
      if (can_redirect || entry.filename == path)
        return entry;
      if (entry.compdb_size) {
        best_folder = &folder;
        best = &entry;
      }
      break;
    }
  }

  bool exists = false;
  std::string dir;
  const std::vector<const char *> *extra = nullptr;
  Project::Entry ret;
  for (auto &[root, folder] : root2folder)
    if (StringRef(path).startswith(root))
      for (auto &[dir1, args] : folder.dot_ccls)
        if (StringRef(path).startswith(dir1)) {
          dir = dir1;
          extra = &args;
          if (AppendToCDB(args))
            goto out;
          exists = true;

          ret.root = ret.directory = root;
          ret.filename = path;
          if (args.empty()) {
            ret.args = GetFallback(path);
          } else {
            ret.args = args;
            ret.args.push_back(Intern(path));
          }
          ProjectProcessor(folder).Process(ret);
          for (const std::string &arg : g_config->clang.extraArgs)
            ret.args.push_back(Intern(arg));
          ret.args.push_back(Intern("-working-directory=" + ret.directory));
          return ret;
        }
out:
  if (must_exist && !exists)
    return ret;

  if (!best) {
    int best_score = INT_MIN;
    for (auto &[root, folder] : root2folder) {
      if (dir.size() && !StringRef(path).startswith(dir))
        continue;
      for (const Entry &e : folder.entries)
        if (e.compdb_size) {
          int score = ComputeGuessScore(path, e.filename);
          if (score > best_score) {
            best_score = score;
            best = &e;
            best_folder = &folder;
          }
        }
    }
  }

  ret.is_inferred = true;
  ret.filename = path;
  if (!best) {
    if (ret.root.empty())
      ret.root = g_config->fallbackFolder;
    ret.directory = ret.root;
    ret.args = GetFallback(path);
  } else {
    ret.root = best->root;
    ret.directory = best->directory;
    ret.args = best->args;
    ret.args.resize(best->compdb_size);
    if (extra && extra->size())
      ret.args.insert(ret.args.end(), extra->begin() + 1, extra->end());
    ProjectProcessor(*best_folder).Process(ret);
    for (const std::string &arg : g_config->clang.extraArgs)
      ret.args.push_back(Intern(arg));
    ret.args.push_back(Intern("-working-directory=" + ret.directory));
  }

  return ret;
}

void Project::Index(WorkingFiles *wfiles, RequestId id) {
  auto &gi = g_config->index;
  GroupMatch match(gi.whitelist, gi.blacklist),
      match_i(gi.initialWhitelist, gi.initialBlacklist);
  {
    std::lock_guard lock(mtx);
    for (auto &[root, folder] : root2folder) {
      int i = 0;
      for (const Project::Entry &entry : folder.entries) {
        std::string reason;
        if (match.Matches(entry.filename, &reason) &&
            match_i.Matches(entry.filename, &reason)) {
          bool interactive = wfiles->GetFile(entry.filename) != nullptr;
          pipeline::Index(entry.filename, entry.args,
                          interactive ? IndexMode::Normal
                                      : IndexMode::NonInteractive,
                          false, id);
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
  pipeline::Index("", {}, IndexMode::NonInteractive, false);
}

void Project::IndexRelated(const std::string &path) {
  auto &gi = g_config->index;
  GroupMatch match(gi.whitelist, gi.blacklist);
  std::string stem = sys::path::stem(path);
  std::lock_guard lock(mtx);
  for (auto &[root, folder] : root2folder)
    if (StringRef(path).startswith(root)) {
      for (const Project::Entry &entry : folder.entries) {
        std::string reason;
        if (sys::path::stem(entry.filename) == stem && entry.filename != path &&
            match.Matches(entry.filename, &reason))
          pipeline::Index(entry.filename, entry.args, IndexMode::NonInteractive,
                          true);
      }
      break;
    }
}
} // namespace ccls
