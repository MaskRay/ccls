#include "project.h"

#include "libclangmm/Utility.h"
#include "match.h"
#include "platform.h"
#include "serializer.h"
#include "utils.h"

#include <clang-c/CXCompilationDatabase.h>
#include <doctest/doctest.h>
#include <loguru.hpp>

#include <iostream>
#include <limits>
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

struct ProjectConfig {
  std::unordered_set<std::string> quote_dirs;
  std::unordered_set<std::string> angle_dirs;
  std::vector<std::string> extra_flags;
  std::string project_dir;
};

static const char* kBlacklistMulti[] = {"-MF", "-Xclang"};

// Blacklisted flags which are always removed from the command line.
static const char* kBlacklist[] = {
    "--param", "-M", "-MD", "-MG", "-MM", "-MMD", "-MP", "-MQ", "-MT", "-Og",
    "-Wa,--32", "-Wa,--64", "-Wl,--incremental-full",
    "-Wl,--incremental-patch,1", "-Wl,--no-incremental",
    "-fbuild-session-file=", "-fbuild-session-timestamp=", "-fembed-bitcode",
    "-fembed-bitcode-marker", "-fmodules-validate-once-per-build-session",
    "-fno-delete-null-pointer-checks",
    "-fno-use-linker-plugin"
    "-fno-var-tracking",
    "-fno-var-tracking-assignments", "-fno-enforce-eh-specs", "-fvar-tracking",
    "-fvar-tracking-assignments", "-fvar-tracking-assignments-toggle",
    "-gcc-toolchain",
    "-march=", "-masm=", "-mcpu=", "-mfpmath=", "-mtune=", "-s",

    "-B",
    //"-f",
    //"-pipe",
    //"-W",
    // TODO: make sure we consume includes before stripping all path-like args.
    "/work/goma/gomacc",
    "../../third_party/llvm-build/Release+Asserts/bin/clang++",
    "-Wno-unused-lambda-capture", "/", "..",
    //"-stdlib=libc++"
};

// Arguments which are followed by a potentially relative path. We need to make
// all relative paths absolute, otherwise libclang will not resolve them.
const char* kPathArgs[] = {"-I", "-iquote", "-isystem", "--sysroot="};

const char* kQuoteIncludeArgs[] = {"-iquote"};
const char* kAngleIncludeArgs[] = {"-I", "-isystem"};

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
    ProjectConfig* config,
    const CompileCommandsEntry& entry) {
  Project::Entry result;
  result.filename = NormalizePath(entry.file);

  bool make_next_flag_absolute = false;
  bool add_next_flag_quote = false;
  bool add_next_flag_angle = false;

  result.args.reserve(entry.args.size() + config->extra_flags.size());
  for (size_t i = 0; i < entry.args.size(); ++i) {
    std::string arg = entry.args[i];

    // If blacklist skip.
    if (std::any_of(
            std::begin(kBlacklistMulti), std::end(kBlacklistMulti),
            [&arg](const char* value) { return StartsWith(arg, value); })) {
      ++i;
      continue;
    }
    if (std::any_of(
            std::begin(kBlacklist), std::end(kBlacklist),
            [&arg](const char* value) { return StartsWith(arg, value); })) {
      continue;
    }

    // Cleanup path for previous argument.
    if (make_next_flag_absolute) {
      if (arg.size() > 0 && arg[0] != '/')
        arg = NormalizePath(entry.directory + arg);
      make_next_flag_absolute = false;

      if (add_next_flag_quote)
        config->quote_dirs.insert(arg);
      if (add_next_flag_angle)
        config->angle_dirs.insert(arg);
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
          config->quote_dirs.insert(path);
        if (ShouldAddToAngleIncludes(arg))
          config->angle_dirs.insert(path);
        break;
      }
    }

    result.args.push_back(arg);
  }

  // We don't do any special processing on user-given extra flags.
  for (const auto& flag : config->extra_flags)
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

std::vector<Project::Entry> LoadFromDirectoryListing(
    ProjectConfig* config) {
  std::vector<Project::Entry> result;

  std::vector<std::string> args;
  std::cerr << "Using arguments: ";
  for (const std::string& line : ReadLines(config->project_dir + "/clang_args")) {
    if (line.empty() || StartsWith(line, "#"))
      continue;
    if (!args.empty())
      std::cerr << ", ";
    std::cerr << line;
    args.push_back(line);
  }
  std::cerr << std::endl;

  std::vector<std::string> files = GetFilesInFolder(
      config->project_dir, true /*recursive*/, true /*add_folder_to_path*/);
  for (const std::string& file : files) {
    if (EndsWith(file, ".cc") || EndsWith(file, ".cpp") ||
        EndsWith(file, ".c")) {
      CompileCommandsEntry e;
      e.file = NormalizePath(file);
      e.args = args;
      result.push_back(
          GetCompilationEntryFromCompileCommandEntry(config, e));
    }
  }

  return result;
}

std::vector<Project::Entry> LoadCompilationEntriesFromDirectory(
    ProjectConfig* config) {

  // Try to load compile_commands.json, but fallback to a project listing.
  LOG_S(INFO) << "Trying to load compile_commands.json";
  CXCompilationDatabase_Error cx_db_load_error;
  CXCompilationDatabase cx_db = clang_CompilationDatabase_fromDirectory(
      config->project_dir.c_str(), &cx_db_load_error);
  if (cx_db_load_error == CXCompilationDatabase_CanNotLoadDatabase) {
    LOG_S(INFO) << "Unable to load compile_commands.json located at \""
              << config->project_dir << "\"; using directory listing instead.";
    return LoadFromDirectoryListing(config);
  }

  CXCompileCommands cx_commands =
      clang_CompilationDatabase_getAllCompileCommands(cx_db);

  unsigned int num_commands = clang_CompileCommands_getSize(cx_commands);
  std::vector<Project::Entry> result;
  for (unsigned int i = 0; i < num_commands; i++) {
    CXCompileCommand cx_command =
        clang_CompileCommands_getCommand(cx_commands, i);

    std::string directory =
        clang::ToString(clang_CompileCommand_getDirectory(cx_command));
    std::string relative_filename =
        clang::ToString(clang_CompileCommand_getFilename(cx_command));
    std::string absolute_filename = directory + "/" + relative_filename;

    CompileCommandsEntry entry;
    entry.file = NormalizePath(absolute_filename);
    entry.directory = directory;

    unsigned num_args = clang_CompileCommand_getNumArgs(cx_command);
    entry.args.reserve(num_args);
    for (unsigned j = 0; j < num_args; ++j)
      entry.args.push_back(
          clang::ToString(clang_CompileCommand_getArg(cx_command, j)));

    result.push_back(GetCompilationEntryFromCompileCommandEntry(
        config, entry));
  }

  clang_CompileCommands_dispose(cx_commands);
  clang_CompilationDatabase_dispose(cx_db);

  return result;
}

// Computes a score based on how well |a| and |b| match. This is used for
// argument guessing.
int ComputeGuessScore(const std::string& a, const std::string& b) {
  const int kMatchPrefixWeight = 100;
  const int kMismatchDirectoryWeight = 100;
  const int kMatchPostfixWeight = 1;

  int score = 0;
  int i = 0;

  // Increase score based on matching prefix.
  for (i = 0; i < a.length() && i < b.length(); ++i) {
    if (a[i] != b[i])
      break;
    score += kMatchPrefixWeight;
  }

  // Reduce score based on mismatched directory distance.
  for (int j = i; j < a.length(); ++j) {
    if (a[j] == '/')
      score -= kMismatchDirectoryWeight;
  }
  for (int j = i; j < b.length(); ++j) {
    if (b[j] == '/')
      score -= kMismatchDirectoryWeight;
  }

  // Increase score based on common ending. Don't increase as much as matching
  // prefix or directory distance.
  for (int offset = 1; offset <= a.length() && offset <= b.length(); ++offset) {
    if (a[a.size() - offset] != b[b.size() - offset])
      break;
    score += kMatchPostfixWeight;
  }

  return score;
}

}  // namespace

void Project::Load(const std::vector<std::string>& extra_flags,
                   const std::string& directory) {
  // Load data.
  ProjectConfig config;
  config.extra_flags = extra_flags;
  config.project_dir = directory;
  entries = LoadCompilationEntriesFromDirectory(&config);

  // Cleanup / postprocess include directories.
  quote_include_directories.assign(config.quote_dirs.begin(),
                                   config.quote_dirs.end());
  angle_include_directories.assign(config.angle_dirs.begin(),
                                   config.angle_dirs.end());
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
  if (best_entry)
    result.args = best_entry->args;
  return result;
}

void Project::ForAllFilteredFiles(
    Config* config,
    std::function<void(int i, const Entry& entry)> action) {
  GroupMatch matcher(config->indexWhitelist, config->indexBlacklist);
  for (int i = 0; i < entries.size(); ++i) {
    const Project::Entry& entry = entries[i];
    std::string failure_reason;
    if (matcher.IsMatch(entry.filename, &failure_reason))
      action(i, entries[i]);
    else {
      if (config->logSkippedPathsForIndex) {
        LOG_S(INFO) << "[" << i + 1 << "/" << entries.size() << "]: Failed "
                    << failure_reason << "; skipping " << entry.filename;
      }
    }
  }
}

TEST_SUITE("Project");

#if false
TEST_CASE("chromium") {
  std::string compile_commands = R"(
    [
      {
        "directory": "/work2/chrome/src/out/Release",
        "command": "/work/goma/gomacc ../../third_party/llvm-build/Release+Asserts/bin/clang++ -MMD -MF obj/apps/apps/app_lifetime_monitor.o.d -DV8_DEPRECATION_WARNINGS -DDCHECK_ALWAYS_ON=1 -DUSE_UDEV -DUSE_ASH=1 -DUSE_AURA=1 -DUSE_NSS_CERTS=1 -DUSE_OZONE=1 -DDISABLE_NACL -DFULL_SAFE_BROWSING -DSAFE_BROWSING_CSD -DSAFE_BROWSING_DB_LOCAL -DCHROMIUM_BUILD -DFIELDTRIAL_TESTING_ENABLED -DCR_CLANG_REVISION=\"310694-1\" -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -DCOMPONENT_BUILD -DOS_CHROMEOS -DNDEBUG -DNVALGRIND -DDYNAMIC_ANNOTATIONS_ENABLED=0 -DGL_GLEXT_PROTOTYPES -DUSE_GLX -DUSE_EGL -DANGLE_ENABLE_RELEASE_ASSERTS -DTOOLKIT_VIEWS=1 -DV8_USE_EXTERNAL_STARTUP_DATA -DU_USING_ICU_NAMESPACE=0 -DU_ENABLE_DYLOAD=0 -DICU_UTIL_DATA_IMPL=ICU_UTIL_DATA_FILE -DUCHAR_TYPE=uint16_t -DGOOGLE_PROTOBUF_NO_RTTI -DGOOGLE_PROTOBUF_NO_STATIC_INITIALIZER -DHAVE_PTHREAD -DPROTOBUF_USE_DLLS -DSK_IGNORE_LINEONLY_AA_CONVEX_PATH_OPTS -DSK_HAS_PNG_LIBRARY -DSK_HAS_WEBP_LIBRARY -DSK_HAS_JPEG_LIBRARY -DSKIA_DLL -DGR_GL_IGNORE_ES3_MSAA=0 -DSK_SUPPORT_GPU=1 -DMESA_EGL_NO_X11_HEADERS -DBORINGSSL_SHARED_LIBRARY -DUSING_V8_SHARED -I../.. -Igen -I../../third_party/libwebp/src -I../../third_party/khronos -I../../gpu -I../../third_party/ced/src -I../../third_party/icu/source/common -I../../third_party/icu/source/i18n -I../../third_party/protobuf/src -I../../skia/config -I../../skia/ext -I../../third_party/skia/include/c -I../../third_party/skia/include/config -I../../third_party/skia/include/core -I../../third_party/skia/include/effects -I../../third_party/skia/include/encode -I../../third_party/skia/include/gpu -I../../third_party/skia/include/images -I../../third_party/skia/include/lazy -I../../third_party/skia/include/pathops -I../../third_party/skia/include/pdf -I../../third_party/skia/include/pipe -I../../third_party/skia/include/ports -I../../third_party/skia/include/utils -I../../third_party/skia/third_party/vulkan -I../../third_party/skia/src/gpu -I../../third_party/skia/src/sksl -I../../third_party/mesa/src/include -I../../third_party/libwebm/source -I../../third_party/protobuf/src -Igen/protoc_out -I../../third_party/boringssl/src/include -I../../build/linux/debian_jessie_amd64-sysroot/usr/include/nss -I../../build/linux/debian_jessie_amd64-sysroot/usr/include/nspr -Igen -I../../third_party/WebKit -Igen/third_party/WebKit -I../../v8/include -Igen/v8/include -Igen -I../../third_party/flatbuffers/src/include -Igen -fno-strict-aliasing -Wno-builtin-macro-redefined -D__DATE__= -D__TIME__= -D__TIMESTAMP__= -funwind-tables -fPIC -pipe -B../../third_party/binutils/Linux_x64/Release/bin -pthread -fcolor-diagnostics -m64 -march=x86-64 -Wall -Werror -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Wno-c++11-narrowing -Wno-covered-switch-default -Wno-unneeded-internal-declaration -Wno-inconsistent-missing-override -Wno-undefined-var-template -Wno-nonportable-include-path -Wno-address-of-packed-member -Wno-unused-lambda-capture -Wno-user-defined-warnings -Wno-enum-compare-switch -O2 -fno-ident -fdata-sections -ffunction-sections -fno-omit-frame-pointer -g0 -fvisibility=hidden -Xclang -load -Xclang ../../third_party/llvm-build/Release+Asserts/lib/libFindBadConstructs.so -Xclang -add-plugin -Xclang find-bad-constructs -Xclang -plugin-arg-find-bad-constructs -Xclang check-auto-raw-pointer -Xclang -plugin-arg-find-bad-constructs -Xclang check-ipc -Wheader-hygiene -Wstring-conversion -Wtautological-overlap-compare -Wexit-time-destructors -Wno-header-guard -Wno-exit-time-destructors -std=gnu++14 -fno-rtti -nostdinc++ -isystem../../buildtools/third_party/libc++/trunk/include -isystem../../buildtools/third_party/libc++abi/trunk/include --sysroot=../../build/linux/debian_jessie_amd64-sysroot -fno-exceptions -fvisibility-inlines-hidden -c ../../apps/app_lifetime_monitor.cc -o obj/apps/apps/app_lifetime_monitor.o",
        "file": "../../apps/app_lifetime_monitor.cc"
      }
    ]
  )";

  std::string project_directory = "/work2/chrome/src/";
  std::vector<std::string> extra_flags;

  std::unordered_set<std::string> quote_includes;
  std::unordered_set<std::string> angle_includes;
  std::vector<Project::Entry> result;

  REQUIRE(TryLoadFromCompileCommandsJson(&result, &quote_includes, &angle_includes, project_directory, extra_flags, compile_commands));

  std::vector<std::string> expected_args{
    "-DV8_DEPRECATION_WARNINGS", "-DDCHECK_ALWAYS_ON=1", "-DUSE_UDEV", "-DUSE_ASH=1", "-DUSE_AURA=1", "-DUSE_NSS_CERTS=1", "-DUSE_OZONE=1", "-DDISABLE_NACL", "-DFULL_SAFE_BROWSING", "-DSAFE_BROWSING_CSD", "-DSAFE_BROWSING_DB_LOCAL", "-DCHROMIUM_BUILD", "-DFIELDTRIAL_TESTING_ENABLED", "-DCR_CLANG_REVISION=\"310694-1\"", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-D_LARGEFILE64_SOURCE", "-D__STDC_CONSTANT_MACROS", "-D__STDC_FORMAT_MACROS", "-DCOMPONENT_BUILD", "-DOS_CHROMEOS", "-DNDEBUG", "-DNVALGRIND", "-DDYNAMIC_ANNOTATIONS_ENABLED=0", "-DGL_GLEXT_PROTOTYPES", "-DUSE_GLX", "-DUSE_EGL", "-DANGLE_ENABLE_RELEASE_ASSERTS", "-DTOOLKIT_VIEWS=1", "-DV8_USE_EXTERNAL_STARTUP_DATA", "-DU_USING_ICU_NAMESPACE=0", "-DU_ENABLE_DYLOAD=0", "-DICU_UTIL_DATA_IMPL=ICU_UTIL_DATA_FILE", "-DUCHAR_TYPE=uint16_t", "-DGOOGLE_PROTOBUF_NO_RTTI", "-DGOOGLE_PROTOBUF_NO_STATIC_INITIALIZER", "-DHAVE_PTHREAD", "-DPROTOBUF_USE_DLLS", "-DSK_IGNORE_LINEONLY_AA_CONVEX_PATH_OPTS", "-DSK_HAS_PNG_LIBRARY", "-DSK_HAS_WEBP_LIBRARY", "-DSK_HAS_JPEG_LIBRARY", "-DSKIA_DLL", "-DGR_GL_IGNORE_ES3_MSAA=0", "-DSK_SUPPORT_GPU=1", "-DMESA_EGL_NO_X11_HEADERS", "-DBORINGSSL_SHARED_LIBRARY", "-DUSING_V8_SHARED", "-IC:/work2/chrome/src", "-IC:/work2/chrome/src/out/Release/gen", "-IC:/work2/chrome/src/third_party/libwebp/src", "-IC:/work2/chrome/src/third_party/khronos", "-IC:/work2/chrome/src/gpu", "-IC:/work2/chrome/src/third_party/ced/src", "-IC:/work2/chrome/src/third_party/icu/source/common", "-IC:/work2/chrome/src/third_party/icu/source/i18n", "-IC:/work2/chrome/src/third_party/protobuf/src", "-IC:/work2/chrome/src/skia/config", "-IC:/work2/chrome/src/skia/ext", "-IC:/work2/chrome/src/third_party/skia/include/c", "-IC:/work2/chrome/src/third_party/skia/include/config", "-IC:/work2/chrome/src/third_party/skia/include/core", "-IC:/work2/chrome/src/third_party/skia/include/effects", "-IC:/work2/chrome/src/third_party/skia/include/encode", "-IC:/work2/chrome/src/third_party/skia/include/gpu", "-IC:/work2/chrome/src/third_party/skia/include/images", "-IC:/work2/chrome/src/third_party/skia/include/lazy", "-IC:/work2/chrome/src/third_party/skia/include/pathops", "-IC:/work2/chrome/src/third_party/skia/include/pdf", "-IC:/work2/chrome/src/third_party/skia/include/pipe", "-IC:/work2/chrome/src/third_party/skia/include/ports", "-IC:/work2/chrome/src/third_party/skia/include/utils", "-IC:/work2/chrome/src/third_party/skia/third_party/vulkan", "-IC:/work2/chrome/src/third_party/skia/src/gpu", "-IC:/work2/chrome/src/third_party/skia/src/sksl", "-IC:/work2/chrome/src/third_party/mesa/src/include", "-IC:/work2/chrome/src/third_party/libwebm/source", "-IC:/work2/chrome/src/third_party/protobuf/src", "-IC:/work2/chrome/src/out/Release/gen/protoc_out", "-IC:/work2/chrome/src/third_party/boringssl/src/include", "-IC:/work2/chrome/src/build/linux/debian_jessie_amd64-sysroot/usr/include/nss", "-IC:/work2/chrome/src/build/linux/debian_jessie_amd64-sysroot/usr/include/nspr", "-IC:/work2/chrome/src/out/Release/gen", "-IC:/work2/chrome/src/third_party/WebKit", "-IC:/work2/chrome/src/out/Release/gen/third_party/WebKit", "-IC:/work2/chrome/src/v8/include", "-IC:/work2/chrome/src/out/Release/gen/v8/include", "-IC:/work2/chrome/src/out/Release/gen", "-IC:/work2/chrome/src/third_party/flatbuffers/src/include", "-IC:/work2/chrome/src/out/Release/gen", "-fno-strict-aliasing", "-Wno-builtin-macro-redefined", "-D__DATE__=", "-D__TIME__=", "-D__TIMESTAMP__=", "-funwind-tables", "-fPIC", "-pipe", "-pthread", "-fcolor-diagnostics", "-m64", "-Wall", "-Werror", "-Wextra", "-Wno-missing-field-initializers", "-Wno-unused-parameter", "-Wno-c++11-narrowing", "-Wno-covered-switch-default", "-Wno-unneeded-internal-declaration", "-Wno-inconsistent-missing-override", "-Wno-undefined-var-template", "-Wno-nonportable-include-path", "-Wno-address-of-packed-member", "-Wno-user-defined-warnings", "-Wno-enum-compare-switch", "-O2", "-fno-ident", "-fdata-sections", "-ffunction-sections", "-fno-omit-frame-pointer", "-g0", "-fvisibility=hidden", "-Wheader-hygiene", "-Wstring-conversion", "-Wtautological-overlap-compare", "-Wexit-time-destructors", "-Wno-header-guard", "-Wno-exit-time-destructors", "-std=gnu++14", "-fno-rtti", "-nostdinc++", "-isystemC:/work2/chrome/src/buildtools/third_party/libc++/trunk/include", "-isystemC:/work2/chrome/src/buildtools/third_party/libc++abi/trunk/include", "--sysroot=C:/work2/chrome/src/build/linux/debian_jessie_amd64-sysroot", "-fno-exceptions", "-fvisibility-inlines-hidden", "-c", "-o", "obj/apps/apps/app_lifetime_monitor.o", "-xc++"
    //"-DV8_DEPRECATION_WARNINGS"
  };
  std::cout << "Expected - Actual\n\n";
  for (int i = 0; i < std::min(result[0].args.size(), expected_args.size()); ++i) {
    if (result[0].args[i] != expected_args[i])
      std::cout << "mismatch at " << i << "; expected " << expected_args[i] << " but got " << result[0].args[i] << std::endl;
  }
  //std::cout << StringJoin(expected_args) << "\n";
  //std::cout << StringJoin(result[0].args) << "\n";
  REQUIRE(result.size() == 1);
  REQUIRE(result[0].args == expected_args);
}

TEST_CASE("argument parsing") {
  std::string compile_commands = R"(
    [
      {
        "directory": "/usr/local/google/code/naive/build",
        "command": "clang++    -std=c++14 -Werror -I/usr/include/pixman-1 -I/usr/include/libdrm -I/usr/local/google/code/naive/src    -o CMakeFiles/naive.dir/src/wm/window.cc.o -c /usr/local/google/code/naive/src/wm/window.cc",
        "file": "/usr/local/google/code/naive/src/wm/window.cc"
      }
    ]
  )";

  std::string project_directory = "/usr/local/google/code/naive/src/";
  std::vector<std::string> extra_flags;

  std::unordered_set<std::string> quote_includes;
  std::unordered_set<std::string> angle_includes;
  std::vector<Project::Entry> result;

  REQUIRE(TryLoadFromCompileCommandsJson(&result, &quote_includes, &angle_includes, project_directory, extra_flags, compile_commands));

  std::vector<std::string> expected_args{
      "-std=c++14",
      "-Werror",
      "-I/usr/include/pixman-1",
      "-I/usr/include/libdrm",
      "-I/usr/local/google/code/naive/src",
      "-o",
      "CMakeFiles/naive.dir/src/wm/window.cc.o",
      "-c",
      "/usr/local/google/code/naive/src/wm/window.cc",
      "-xc++"
  };
  REQUIRE(result.size() == 1);
  REQUIRE(result[0].args == expected_args);
}

TEST_CASE("relative directories") {
  std::string compile_commands = R"(
    [
      {
        "directory": "/build",
        "command": "clang++ -std=c++14 -Werror",
        "file": "bar/foo.cc"
      }
    ]
  )";

  std::string project_directory = "/build/";
  std::vector<std::string> extra_flags;

  std::unordered_set<std::string> quote_includes;
  std::unordered_set<std::string> angle_includes;
  std::vector<Project::Entry> result;

  REQUIRE(TryLoadFromCompileCommandsJson(&result, &quote_includes, &angle_includes, project_directory, extra_flags, compile_commands));
  REQUIRE(result.size() == 1);
  REQUIRE(result[0].filename == "/build/bar/foo.cc");
}

TEST_CASE("Directory extraction") {
  std::string build_directory = "/base/";
  std::string filename = "foo.cc";
  std::vector<std::string> args = { "-I/absolute1", "-I", "/absolute2", "-Irelative1", "-I", "relative2" };
  std::unordered_set<std::string> angle_includes;
  std::unordered_set<std::string> quote_includes;
  CleanupArguments(build_directory, filename, &args, &angle_includes, &quote_includes);

  std::unordered_set<std::string> expected{ "/absolute1", "/absolute2", "/base/relative1", "/base/relative2" };
  REQUIRE(angle_includes == expected);
}
#endif

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

TEST_SUITE_END();
