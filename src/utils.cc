#include "utils.h"

#include "filesystem.hh"
#include "platform.h"

#include <doctest/doctest.h>
#include <siphash.h>
#include <loguru/loguru.hpp>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <fstream>
#include <functional>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>

// DEFAULT_RESOURCE_DIRECTORY is passed with quotes for non-MSVC compilers, ie,
// foo vs "foo".
#if defined(_MSC_VER)
#define _STRINGIFY(x) #x
#define ENSURE_STRING_MACRO_ARGUMENT(x) _STRINGIFY(x)
#else
#define ENSURE_STRING_MACRO_ARGUMENT(x) x
#endif

void TrimInPlace(std::string& s) {
  s.erase(s.begin(),
          std::find_if(s.begin(), s.end(),
                       std::not1(std::ptr_fun<int, int>(std::isspace))));
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       std::not1(std::ptr_fun<int, int>(std::isspace)))
              .base(),
          s.end());
}
std::string Trim(std::string s) {
  TrimInPlace(s);
  return s;
}
void RemoveLastCR(std::string& s) {
  if (!s.empty() && *s.rbegin() == '\r')
    s.pop_back();
}

uint64_t HashUsr(const std::string& s) {
  return HashUsr(s.c_str(), s.size());
}

uint64_t HashUsr(const char* s) {
  return HashUsr(s, strlen(s));
}

uint64_t HashUsr(const char* s, size_t n) {
  union {
    uint64_t ret;
    uint8_t out[8];
  };
  // k is an arbitrary key. Don't change it.
  const uint8_t k[16] = {0xd0, 0xe5, 0x4d, 0x61, 0x74, 0x63, 0x68, 0x52,
                         0x61, 0x79, 0xea, 0x70, 0xca, 0x70, 0xf0, 0x0d};
  (void)siphash(reinterpret_cast<const uint8_t*>(s), n, k, out, 8);
  return ret;
}

// See http://stackoverflow.com/a/2072890
bool EndsWith(std::string_view value, std::string_view ending) {
  if (ending.size() > value.size())
    return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

bool StartsWith(std::string_view value, std::string_view start) {
  if (start.size() > value.size())
    return false;
  return std::equal(start.begin(), start.end(), value.begin());
}

bool AnyStartsWith(const std::vector<std::string>& values,
                   const std::string& start) {
  return std::any_of(
      std::begin(values), std::end(values),
      [&start](const std::string& value) { return StartsWith(value, start); });
}

bool StartsWithAny(const std::string& value,
                   const std::vector<std::string>& startings) {
  return std::any_of(std::begin(startings), std::end(startings),
                     [&value](const std::string& starting) {
                       return StartsWith(value, starting);
                     });
}

bool EndsWithAny(const std::string& value,
                 const std::vector<std::string>& endings) {
  return std::any_of(
      std::begin(endings), std::end(endings),
      [&value](const std::string& ending) { return EndsWith(value, ending); });
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
  size_t last_slash = path.find_last_of('/');
  if (last_slash != std::string::npos && (last_slash + 1) < path.size())
    return path.substr(last_slash + 1);
  return path;
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

std::string LowerPathIfCaseInsensitive(const std::string& path) {
  std::string result = path;
#if defined(_WIN32)
  for (size_t i = 0; i < result.size(); ++i)
    result[i] = (char)tolower(result[i]);
#endif
  return result;
}

static void GetFilesInFolderHelper(
    std::string folder,
    bool recursive,
    std::string output_prefix,
    const std::function<void(const std::string&)>& handler) {
  std::queue<std::pair<fs::path, fs::path>> q;
  q.push(std::make_pair(fs::path(folder), fs::path(output_prefix)));
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
  if (path.size() && path.back() == '/')
    path.pop_back();
  std::replace(path.begin(), path.end(), '\\', '@');
  std::replace(path.begin(), path.end(), '/', '@');
  std::replace(path.begin(), path.end(), ':', '@');
  return path;
}

// http://stackoverflow.com/a/6089413
std::istream& SafeGetline(std::istream& is, std::string& t) {
  t.clear();

  // The characters in the stream are read one-by-one using a std::streambuf.
  // That is faster than reading them one-by-one using the std::istream. Code
  // that uses streambuf this way must be guarded by a sentry object. The sentry
  // object performs various tasks, such as thread synchronization and updating
  // the stream state.

  std::istream::sentry se(is, true);
  std::streambuf* sb = is.rdbuf();

  for (;;) {
    int c = sb->sbumpc();
    if (c == EOF) {
      // Also handle the case when the last line has no line ending
      if (t.empty())
        is.setstate(std::ios::eofbit);
      return is;
    }

    t += (char)c;

    if (c == '\n')
      return is;
  }
}

bool FileExists(const std::string& filename) {
  return fs::exists(filename);
}

std::optional<std::string> ReadContent(const std::string& filename) {
  LOG_S(INFO) << "Reading " << filename;
  std::ifstream cache;
  cache.open(filename);

  try {
    return std::string(std::istreambuf_iterator<char>(cache),
                       std::istreambuf_iterator<char>());
  } catch (std::ios_base::failure&) {
    return std::nullopt;
  }
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
  std::ofstream file(filename,
                     std::ios::out | std::ios::trunc | std::ios::binary);
  if (!file.good()) {
    LOG_S(ERROR) << "Cannot write to " << filename;
    return;
  }

  file << content;
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

TEST_SUITE("StripFileType") {
  TEST_CASE("all") {
    REQUIRE(StripFileType("") == "");
    REQUIRE(StripFileType("bar") == "bar");
    REQUIRE(StripFileType("bar.cc") == "bar");
    REQUIRE(StripFileType("foo/bar.cc") == "foo/bar");
  }
}
