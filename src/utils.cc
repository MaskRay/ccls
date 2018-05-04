#include "utils.h"

#include "filesystem.hh"
#include "platform.h"

#include <doctest/doctest.h>
#include <siphash.h>
#include <loguru/loguru.hpp>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <algorithm>
#include <functional>
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

std::optional<std::string> ReadContent(const std::string& filename) {
  LOG_S(INFO) << "read " << filename;
  char buf[4096];
  std::string ret;
  FILE* f = fopen(filename.c_str(), "rb");
  if (!f) return {};
  size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0)
    ret.append(buf, n);
  fclose(f);
  return ret;
}

void WriteToFile(const std::string& filename, const std::string& content) {
  FILE* f = fopen(filename.c_str(), "wb");
  if (!f || fwrite(content.c_str(), content.size(), 1, f) != 1) {
    LOG_S(ERROR) << "Failed to write to " << filename << ' ' << strerror(errno);
    return;
  }
  fclose(f);
}

std::optional<int64_t> LastWriteTime(const std::string& filename) {
  std::error_code ec;
  auto ftime = fs::last_write_time(filename, ec);
  if (ec) return std::nullopt;
  return std::chrono::time_point_cast<std::chrono::nanoseconds>(ftime)
      .time_since_epoch()
      .count();
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
