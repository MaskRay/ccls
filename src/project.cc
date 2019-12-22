// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

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
#include <llvm/Support/GlobPattern.h>
#include <llvm/Support/LineIterator.h>
#include <llvm/Support/Program.h>

#include <rapidjson/writer.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
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
  auto i = types::lookupTypeForExtension(
      sys::path::extension({filename.data(), filename.size()}).substr(1));
  bool header = i == types::TY_CHeader || i == types::TY_CXXHeader ||
                i == types::TY_ObjCXXHeader;
  bool objc = types::isObjC(i);
  LanguageId ret;
  if (types::isCXX(i))
    ret = types::isCuda(i) ? LanguageId::Cuda
                           : objc ? LanguageId::ObjCpp : LanguageId::Cpp;
  else if (objc)
    ret = LanguageId::ObjC;
  else if (i == types::TY_C || i == types::TY_CHeader)
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
  StringSet<> exclude_args;
  std::vector<GlobPattern> exclude_globs;

  ProjectProcessor(Project::Folder &folder) : folder(folder) {
    for (auto &arg : g_config->clang.excludeArgs)
      if (arg.find_first_of("?*[") == std::string::npos)
        exclude_args.insert(arg);
      else if (Expected<GlobPattern> glob_or_err = GlobPattern::create(arg))
        exclude_globs.push_back(std::move(*glob_or_err));
      else
        LOG_S(WARNING) << toString(glob_or_err.takeError());
  }

  bool excludesArg(StringRef arg, int &i) {
    if (arg.startswith("-M")) {
      if (arg == "-MF" || arg == "-MT" || arg == "-MQ")
        i++;
      return true;
    }
    if (arg == "-Xclang") {
      i++;
      return true;
    }
    return exclude_args.count(arg) ||
           any_of(exclude_globs,
                  [&](const GlobPattern &glob) { return glob.match(arg); });
  }

  // Expand %c %cpp ... in .ccls
  void process(Project::Entry &entry) {
    std::vector<const char *> args(entry.args.begin(),
                                   entry.args.begin() + entry.compdb_size);
    auto [lang, header] = lookupExtension(entry.filename);
    for (int i = entry.compdb_size; i < entry.args.size(); i++) {
      const char *arg = entry.args[i];
      StringRef a(arg);
      if (a[0] == '%') {
        bool ok = false;
        for (;;) {
          if (a.consume_front("%c "))
            ok |= lang == LanguageId::C;
          else if (a.consume_front("%h "))
            ok |= lang == LanguageId::C && header;
          else if (a.consume_front("%cpp "))
            ok |= lang == LanguageId::Cpp;
          else if (a.consume_front("%cu "))
            ok |= lang == LanguageId::Cuda;
          else if (a.consume_front("%hpp "))
            ok |= lang == LanguageId::Cpp && header;
          else if (a.consume_front("%objective-c "))
            ok |= lang == LanguageId::ObjC;
          else if (a.consume_front("%objective-cpp "))
            ok |= lang == LanguageId::ObjCpp;
          else
            break;
        }
        if (ok)
          args.push_back(a.data());
      } else if (!excludesArg(a, i)) {
        args.push_back(arg);
      }
    }
    entry.args = args;
    getSearchDirs(entry);
  }

  void getSearchDirs(Project::Entry &entry) {
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
      args.push_back(intern(arg));
    args.push_back(intern("-working-directory=" + entry.directory));
    args.push_back(intern("-resource-dir=" + g_config->clang.resourceDir));

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
          normalizePath(resolveIfRelative(entry.directory, E.Path));
      ensureEndsInSlash(path);
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
readCompilerArgumentsFromFile(const std::string &path) {
  auto mbOrErr = MemoryBuffer::getFile(path);
  if (!mbOrErr)
    return {};
  std::vector<const char *> args;
  for (line_iterator i(*mbOrErr.get(), true, '#'), e; i != e; ++i) {
    std::string line = *i;
    doPathMapping(line);
    args.push_back(intern(line));
  }
  return args;
}

bool appendToCDB(const std::vector<const char *> &args) {
  return args.size() && StringRef("%compile_commands.json") == args[0];
}

std::vector<const char *> getFallback(const std::string &path) {
  std::vector<const char *> argv{"clang"};
  if (sys::path::extension(path) == ".h")
    argv.push_back("-xobjective-c++-header");
  argv.push_back(intern(path));
  return argv;
}

void loadDirectoryListing(ProjectProcessor &proc, const std::string &root,
                          const StringSet<> &seen) {
  Project::Folder &folder = proc.folder;
  std::vector<std::string> files;

  auto getDotCcls = [&root, &folder](std::string cur) {
    while (!(cur = sys::path::parent_path(cur)).empty()) {
      auto it = folder.dot_ccls.find(cur);
      if (it != folder.dot_ccls.end())
        return it->second;
      std::string normalized = normalizePath(cur);
      // Break if outside of the project root.
      if (normalized.size() <= root.size() ||
          normalized.compare(0, root.size(), root) != 0)
        break;
    }
    return folder.dot_ccls[root];
  };

  getFilesInFolder(root, true /*recursive*/, true /*add_folder_to_path*/,
                   [&folder, &files, &seen](const std::string &path) {
                     std::pair<LanguageId, bool> lang = lookupExtension(path);
                     if (lang.first != LanguageId::Unknown && !lang.second) {
                       if (!seen.count(path))
                         files.push_back(path);
                     } else if (sys::path::filename(path) == ".ccls") {
                       std::vector<const char *> args =
                           readCompilerArgumentsFromFile(path);
                       folder.dot_ccls.emplace(
                           sys::path::parent_path(path).str() + '/', args);
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
    if (const auto &args = getDotCcls(e.filename); appendToCDB(args)) {
      if (args.size())
        e.args.insert(e.args.end(), args.begin() + 1, args.end());
      proc.process(e);
    }
  // Set flags for files not in compile_commands.json
  for (const std::string &file : files)
    if (const auto &args = getDotCcls(file); !appendToCDB(args)) {
      Project::Entry e;
      e.root = e.directory = root;
      e.filename = file;
      if (args.empty()) {
        e.args = getFallback(e.filename);
      } else {
        e.args = args;
        e.args.push_back(intern(e.filename));
      }
      proc.process(e);
      folder.entries.push_back(e);
    }
}

// Computes a score based on how well |a| and |b| match. This is used for
// argument guessing.
int computeGuessScore(std::string_view a, std::string_view b) {
  int score = 0;
  unsigned h = 0;
  llvm::SmallDenseMap<unsigned, int> m;
  for (uint8_t c : a)
    if (c == '/') {
      score -= 9;
      if (h)
        m[h]++;
      h = 0;
    } else {
      h = h * 33 + c;
    }
  h = 0;
  for (uint8_t c : b)
    if (c == '/') {
      score -= 9;
      auto it = m.find(h);
      if (it != m.end() && it->second > 0) {
        it->second--;
        score += 31;
      }
      h = 0;
    } else {
      h = h * 33 + c;
    }

  uint8_t c;
  int d[127] = {};
  for (int i = a.size(); i-- && (c = a[i]) != '/';)
    if (c < 127)
      d[c]++;
  for (int i = b.size(); i-- && (c = b[i]) != '/';)
    if (c < 127 && d[c])
      d[c]--, score++;
  return score;
}

} // namespace

void Project::loadDirectory(const std::string &root, Project::Folder &folder) {
  SmallString<256> cdbDir, path, stdinPath;
  std::string err_msg;
  folder.entries.clear();
  if (g_config->compilationDatabaseCommand.empty()) {
    cdbDir = root;
    if (g_config->compilationDatabaseDirectory.size()) {
      if (sys::path::is_absolute(g_config->compilationDatabaseDirectory))
        cdbDir = g_config->compilationDatabaseDirectory;
      else
        sys::path::append(cdbDir, g_config->compilationDatabaseDirectory);
    }
    sys::path::append(path, cdbDir, "compile_commands.json");
  } else {
    // If `compilationDatabaseCommand` is specified, execute it to get the
    // compdb.
#ifdef _WIN32
    char tmpdir[L_tmpnam];
    tmpnam_s(tmpdir, L_tmpnam);
    cdbDir = tmpdir;
    if (sys::fs::create_directory(tmpdir, false))
      return;
#else
    char tmpdir[] = "/tmp/ccls-compdb-XXXXXX";
    if (!mkdtemp(tmpdir))
      return;
    cdbDir = tmpdir;
#endif
    sys::path::append(path, cdbDir, "compile_commands.json");
    sys::path::append(stdinPath, cdbDir, "stdin");
    {
      rapidjson::StringBuffer sb;
      rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
      JsonWriter json_writer(&writer);
      reflect(json_writer, *g_config);
      std::string input = sb.GetString();
      FILE *fout = fopen(stdinPath.c_str(), "wb");
      fwrite(input.c_str(), input.size(), 1, fout);
      fclose(fout);
    }
    std::array<Optional<StringRef>, 3> redir{StringRef(stdinPath),
                                             StringRef(path), StringRef()};
    std::vector<StringRef> args{g_config->compilationDatabaseCommand, root};
    if (sys::ExecuteAndWait(args[0], args, llvm::None, redir, 0, 0, &err_msg) <
        0) {
      LOG_S(ERROR) << "failed to execute " << args[0].str() << " "
                   << args[1].str() << ": " << err_msg;
      return;
    }
  }

  std::unique_ptr<tooling::CompilationDatabase> cdb =
      tooling::CompilationDatabase::loadFromDirectory(cdbDir, err_msg);
  if (!g_config->compilationDatabaseCommand.empty()) {
#ifdef _WIN32
    DeleteFileA(stdinPath.c_str());
    DeleteFileA(path.c_str());
    RemoveDirectoryA(cdbDir.c_str());
#else
    unlink(stdinPath.c_str());
    unlink(path.c_str());
    rmdir(cdbDir.c_str());
#endif
  }

  ProjectProcessor proc(folder);
  StringSet<> seen;
  std::vector<Project::Entry> result;
  if (!cdb) {
    if (g_config->compilationDatabaseCommand.size() || sys::fs::exists(path))
      LOG_S(ERROR) << "failed to load " << path.c_str();
  } else {
    LOG_S(INFO) << "loaded " << path.c_str();
    for (tooling::CompileCommand &cmd : cdb->getAllCompileCommands()) {
      static bool once;
      Project::Entry entry;
      entry.root = root;
      doPathMapping(entry.root);

      // If workspace folder is real/ but entries use symlink/, convert to
      // real/.
      entry.directory = realPath(cmd.Directory);
      normalizeFolder(entry.directory);
      doPathMapping(entry.directory);
      entry.filename =
          realPath(resolveIfRelative(entry.directory, cmd.Filename));
      normalizeFolder(entry.filename);
      doPathMapping(entry.filename);

      std::vector<std::string> args = std::move(cmd.CommandLine);
      entry.args.reserve(args.size());
      for (int i = 0; i < args.size(); i++) {
        doPathMapping(args[i]);
        if (!proc.excludesArg(args[i], i))
          entry.args.push_back(intern(args[i]));
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
      proc.getSearchDirs(entry);

      if (seen.insert(entry.filename).second)
        folder.entries.push_back(entry);
    }
  }

  // Use directory listing if .ccls exists or compile_commands.json does not
  // exist.
  path.clear();
  sys::path::append(path, root, ".ccls");
  if (sys::fs::exists(path))
    loadDirectoryListing(proc, root, seen);
}

void Project::load(const std::string &root) {
  assert(root.back() == '/');
  std::lock_guard lock(mtx);
  Folder &folder = root2folder[root];

  loadDirectory(root, folder);
  for (auto &[path, kind] : folder.search_dir2kind)
    LOG_S(INFO) << "search directory: " << path << ' ' << " \"< "[kind];

  // Setup project entries.
  folder.path2entry_index.reserve(folder.entries.size());
  for (size_t i = 0; i < folder.entries.size(); ++i) {
    folder.entries[i].id = i;
    folder.path2entry_index[folder.entries[i].filename] = i;
  }
}

Project::Entry Project::findEntry(const std::string &path, bool can_redirect,
                                  bool must_exist) {
  std::string best_dot_ccls_root;
  Project::Folder *best_dot_ccls_folder = nullptr;
  std::string best_dot_ccls_dir;
  const std::vector<const char *> *best_dot_ccls_args = nullptr;

  bool match = false, exact_match = false;
  const Entry *best = nullptr;
  Project::Folder *best_compdb_folder = nullptr;

  Project::Entry ret;
  std::lock_guard lock(mtx);

  for (auto &[root, folder] : root2folder)
    if (StringRef(path).startswith(root)) {
      // Find the best-fit .ccls
      for (auto &[dir, args] : folder.dot_ccls)
        if (StringRef(path).startswith(dir) &&
            dir.length() > best_dot_ccls_dir.length()) {
          best_dot_ccls_root = root;
          best_dot_ccls_folder = &folder;
          best_dot_ccls_dir = dir;
          best_dot_ccls_args = &args;
        }

      if (!match) {
        auto it = folder.path2entry_index.find(path);
        if (it != folder.path2entry_index.end()) {
          Project::Entry &entry = folder.entries[it->second];
          exact_match = entry.filename == path;
          if ((match = exact_match || can_redirect) || entry.compdb_size) {
            // best->compdb_size is >0 for a compdb entry, 0 for a .ccls entry.
            best_compdb_folder = &folder;
            best = &entry;
          }
        }
      }
    }

  bool append = false;
  if (best_dot_ccls_args && !(append = appendToCDB(*best_dot_ccls_args)) &&
      !exact_match) {
    // If the first line is not %compile_commands.json, override the compdb
    // match if it is not an exact match.
    ret.root = ret.directory = best_dot_ccls_root;
    ret.filename = path;
    if (best_dot_ccls_args->empty()) {
      ret.args = getFallback(path);
    } else {
      ret.args = *best_dot_ccls_args;
      ret.args.push_back(intern(path));
    }
  } else {
    // If the first line is %compile_commands.json, find the matching compdb
    // entry and append .ccls args.
    if (must_exist && !match && !(best_dot_ccls_args && !append))
      return ret;
    if (!best) {
      // Infer args from a similar path.
      int best_score = INT_MIN;
      auto [lang, header] = lookupExtension(path);
      for (auto &[root, folder] : root2folder)
        if (StringRef(path).startswith(root))
          for (const Entry &e : folder.entries)
            if (e.compdb_size) {
              int score = computeGuessScore(path, e.filename);
              // Decrease score if .c is matched against .hh
              auto [lang1, header1] = lookupExtension(e.filename);
              if (lang != lang1 && !(lang == LanguageId::C && header))
                score -= 30;
              if (score > best_score) {
                best_score = score;
                best_compdb_folder = &folder;
                best = &e;
              }
            }
      ret.is_inferred = true;
    }
    if (!best) {
      ret.root = ret.directory = g_config->fallbackFolder;
      ret.args = getFallback(path);
    } else {
      // The entry may have different filename but it doesn't matter when
      // building CompilerInvocation. The main filename is specified
      // separately.
      ret.root = best->root;
      ret.directory = best->directory;
      ret.args = best->args;
      if (best->compdb_size) // delete trailing .ccls options if exist
        ret.args.resize(best->compdb_size);
      else
        best_dot_ccls_args = nullptr;
    }
    ret.filename = path;
  }

  if (best_dot_ccls_args && append && best_dot_ccls_args->size())
    ret.args.insert(ret.args.end(), best_dot_ccls_args->begin() + 1,
                    best_dot_ccls_args->end());
  if (best_compdb_folder)
    ProjectProcessor(*best_compdb_folder).process(ret);
  else if (best_dot_ccls_folder)
    ProjectProcessor(*best_dot_ccls_folder).process(ret);
  for (const std::string &arg : g_config->clang.extraArgs)
    ret.args.push_back(intern(arg));
  ret.args.push_back(intern("-working-directory=" + ret.directory));
  return ret;
}

void Project::index(WorkingFiles *wfiles, const RequestId &id) {
  auto &gi = g_config->index;
  GroupMatch match(gi.whitelist, gi.blacklist),
      match_i(gi.initialWhitelist, gi.initialBlacklist);
  std::vector<const char *> args, extra_args;
  for (const std::string &arg : g_config->clang.extraArgs)
    extra_args.push_back(intern(arg));
  {
    std::lock_guard lock(mtx);
    for (auto &[root, folder] : root2folder) {
      int i = 0;
      for (const Project::Entry &entry : folder.entries) {
        std::string reason;
        if (match.matches(entry.filename, &reason) &&
            match_i.matches(entry.filename, &reason)) {
          bool interactive = wfiles->getFile(entry.filename) != nullptr;
          args = entry.args;
          args.insert(args.end(), extra_args.begin(), extra_args.end());
          args.push_back(intern("-working-directory=" + entry.directory));
          pipeline::index(entry.filename, args,
                          interactive ? IndexMode::Normal
                                      : IndexMode::Background,
                          false, id);
        } else {
          LOG_V(1) << "[" << i << "/" << folder.entries.size()
                   << "]: " << reason << "; skip " << entry.filename;
        }
        i++;
      }
    }
  }

  pipeline::loaded_ts = pipeline::tick;
  // Dummy request to indicate that project is loaded and
  // trigger refreshing semantic highlight for all working files.
  pipeline::index("", {}, IndexMode::Background, false);
}

void Project::indexRelated(const std::string &path) {
  auto &gi = g_config->index;
  GroupMatch match(gi.whitelist, gi.blacklist);
  std::string stem = sys::path::stem(path);
  std::vector<const char *> args, extra_args;
  for (const std::string &arg : g_config->clang.extraArgs)
    extra_args.push_back(intern(arg));
  std::lock_guard lock(mtx);
  for (auto &[root, folder] : root2folder)
    if (StringRef(path).startswith(root)) {
      for (const Project::Entry &entry : folder.entries) {
        std::string reason;
        args = entry.args;
        args.insert(args.end(), extra_args.begin(), extra_args.end());
        args.push_back(intern("-working-directory=" + entry.directory));
        if (sys::path::stem(entry.filename) == stem && entry.filename != path &&
            match.matches(entry.filename, &reason))
          pipeline::index(entry.filename, args, IndexMode::Background, true);
      }
      break;
    }
}
} // namespace ccls
