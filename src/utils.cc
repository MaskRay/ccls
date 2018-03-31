#include "utils.h"

#include "filesystem.hh"
#include "platform.h"

#include <doctest/doctest.h>
#include <siphash.h>
#include <loguru/loguru.hpp>

#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <algorithm>
#include <fstream>
#include <functional>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
using namespace std::placeholders;

// DEFAULT_RESOURCE_DIRECTORY is passed with quotes for non-MSVC compilers, ie,
// foo vs "foo".
#if defined(_MSC_VER)
#define _STRINGIFY(x) #x
#define ENSURE_STRING_MACRO_ARGUMENT(x) _STRINGIFY(x)
#else
#define ENSURE_STRING_MACRO_ARGUMENT(x) x
#endif

void TrimInPlace(std::string& s) {
  auto f = [](char c) { return !isspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), f));
  s.erase(std::find_if(s.rbegin(), s.rend(), f).base(), s.end());
}
std::string Trim(std::string s) {
  TrimInPlace(s);
  return s;
}

uint64_t HashUsr(std::string_view s) {
  union {
    uint64_t ret;
    uint8_t out[8];
  };
  // k is an arbitrary key. Don't change it.
  const uint8_t k[16] = {0xd0, 0xe5, 0x4d, 0x61, 0x74, 0x63, 0x68, 0x52,
                         0x61, 0x79, 0xea, 0x70, 0xca, 0x70, 0xf0, 0x0d};
  (void)siphash(reinterpret_cast<const uint8_t*>(s.data()), s.size(), k, out, 8);
  return ret;
}

bool EndsWith(std::string_view s, std::string_view suffix) {
  return s.size() >= suffix.size() &&
         std::equal(suffix.rbegin(), suffix.rend(), s.rbegin());
}

bool StartsWith(std::string_view s, std::string_view prefix) {
  return s.size() >= prefix.size() &&
         std::equal(prefix.begin(), prefix.end(), s.begin());
}

bool AnyStartsWith(const std::vector<std::string>& xs,
                   std::string_view prefix) {
  return std::any_of(xs.begin(), xs.end(), std::bind(StartsWith, _1, prefix));
}

bool StartsWithAny(std::string_view s, const std::vector<std::string>& ps) {
  return std::any_of(ps.begin(), ps.end(), std::bind(StartsWith, s, _1));
}

bool EndsWithAny(std::string_view s, const std::vector<std::string>& ss) {
  return std::any_of(ss.begin(), ss.end(), std::bind(EndsWith, s, _1));
}

bool FindAnyPartial(const std::string& value,
                    const std::vector<std::string>& values) {
  return std::any_of(std::begin(values), std::end(values),
                     [&value](const std::string& v) {
                       return value.find(v) != std::string::npos;
                     });
}

std::string GetDirName(std::string path) {
  if (path.size() && path.back() == '/')
    path.pop_back();
  size_t last_slash = path.find_last_of('/');
  if (last_slash == std::string::npos)
    return ".";
  if (last_slash == 0)
    return "/";
  return path.substr(0, last_slash);
}

std::string GetBaseName(const std::string& path) {
  return fs::path(path).filename();
}

std::string StripFileType(const std::string& path) {
  fs::path p(path);
  return p.parent_path() / p.stem();
}

std::vector<std::string> SplitString(const std::string& str,
                                     const std::string& delimiter) {
  // http://stackoverflow.com/a/13172514
  std::vector<std::string> strings;

  std::string::size_type pos = 0;
  std::string::size_type prev = 0;
  while ((pos = str.find(delimiter, prev)) != std::string::npos) {
    strings.push_back(str.substr(prev, pos - prev));
    prev = pos + 1;
  }

  // To get the last substring (or only, if delimiter is not found)
  strings.push_back(str.substr(prev));

  return strings;
}

std::string LowerPathIfInsensitive(const std::string& path) {
#if defined(_WIN32)
  std::string ret = path;
  for (char& c : ret)
    c = tolower(c);
  return ret;
#else
  return path;
#endif
}

static void GetFilesInFolderHelper(
    std::string folder,
    bool recursive,
    std::string output_prefix,
    const std::function<void(const std::string&)>& handler) {
  std::queue<std::pair<fs::path, fs::path>> q;
  q.emplace(fs::path(folder), fs::path(output_prefix));
  while (!q.empty()) {
    for (auto it = fs::directory_iterator(q.front().first); it != fs::directory_iterator(); ++it) {
      auto path = it->path();
      std::string filename = path.filename();
      if (filename[0] != '.' || filename == ".ccls") {
        fs::file_status status = it->symlink_status();
        if (fs::is_regular_file(status))
          handler(q.front().second / filename);
        else if (fs::is_directory(status) || fs::is_symlink(status)) {
          if (recursive) {
            std::string child_dir = q.front().second / filename;
            if (fs::is_directory(status))
              q.push(make_pair(path, child_dir));
          }
        }
      }
    }
    q.pop();
  }
}

std::vector<std::string> GetFilesInFolder(std::string folder,
                                          bool recursive,
                                          bool add_folder_to_path) {
  EnsureEndsInSlash(folder);
  std::vector<std::string> result;
  GetFilesInFolderHelper(
      folder, recursive, add_folder_to_path ? folder : "",
      [&result](const std::string& path) { result.push_back(path); });
  return result;
}

void GetFilesInFolder(std::string folder,
                      bool recursive,
                      bool add_folder_to_path,
                      const std::function<void(const std::string&)>& handler) {
  EnsureEndsInSlash(folder);
  GetFilesInFolderHelper(folder, recursive, add_folder_to_path ? folder : "",
                         handler);
}

void EnsureEndsInSlash(std::string& path) {
  if (path.empty() || path[path.size() - 1] != '/')
    path += '/';
}

std::string EscapeFileName(std::string path) {
  bool slash = path.size() && path.back() == '/';
  for (char& c : path)
    if (c == '\\' || c == '/' || c == ':')
      c = '@';
  if (slash)
    path += '/';
  return path;
}

bool FileExists(const std::string& filename) {
  return fs::exists(filename);
}

std::optional<std::string> ReadContent(const std::string& filename) {
  LOG_S(INFO) << "Reading " << filename;
  char buf[4096];
  std::string ret;
  FILE* f = fopen(filename.c_str(), "rb");
  if (!f) return {};
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0)
    ret.append(buf, n);
  return ret;
}

std::vector<std::string> ReadFileLines(std::string filename) {
  std::vector<std::string> result;
  std::ifstream fin(filename);
  for (std::string line; std::getline(fin, line);)
    result.push_back(line);
  return result;
}

std::vector<std::string> ToLines(const std::string& content) {
  std::vector<std::string> result;
  std::istringstream lines(content);
  std::string line;
  while (getline(lines, line))
    result.push_back(line);
  return result;
}

std::string TextReplacer::Apply(const std::string& content) {
  std::string result = content;

  for (const Replacement& replacement : replacements) {
    while (true) {
      size_t idx = result.find(replacement.from);
      if (idx == std::string::npos)
        break;

      result.replace(result.begin() + idx,
                     result.begin() + idx + replacement.from.size(),
                     replacement.to);
    }
  }

  return result;
}

void WriteToFile(const std::string& filename, const std::string& content) {
  FILE* f = fopen(filename.c_str(), "wb");
  if (!f || fwrite(content.c_str(), content.size(), 1, f) != 1) {
    LOG_S(ERROR) << "Cannot write to " << filename;
    return;
  }
  fclose(f);
}

std::string GetDefaultResourceDirectory() {
  std::string result;

  std::string resource_directory =
      std::string(ENSURE_STRING_MACRO_ARGUMENT(DEFAULT_RESOURCE_DIRECTORY));
  // Remove double quoted resource dir if it was passed with quotes
  // by the build system.
  if (resource_directory.size() >= 2 && resource_directory[0] == '"' &&
      resource_directory[resource_directory.size() - 1] == '"') {
    resource_directory =
        resource_directory.substr(1, resource_directory.size() - 2);
  }
  if (resource_directory.compare(0, 2, "..") == 0) {
    std::string executable_path = GetExecutablePath();
    size_t pos = executable_path.find_last_of('/');
    result = executable_path.substr(0, pos + 1);
    result += resource_directory;
  } else {
    result = resource_directory;
  }

  return NormalizePath(result);
}

void StartThread(const std::string& thread_name, std::function<void()> entry) {
  new std::thread([thread_name, entry]() {
    SetCurrentThreadName(thread_name);
    entry();
  });
}

TEST_SUITE("StripFileType") {
  TEST_CASE("all") {
    REQUIRE(StripFileType("") == "");
    REQUIRE(StripFileType("bar") == "bar");
    REQUIRE(StripFileType("bar.cc") == "bar");
    REQUIRE(StripFileType("foo/bar.cc") == "foo/bar");
  }
}
