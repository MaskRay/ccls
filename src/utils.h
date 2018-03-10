#pragma once

#include <optional.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

// Trim from start (in place)
void TrimStartInPlace(std::string& s);
// Trim from end (in place)
void TrimEndInPlace(std::string& s);
// Trim from both ends (in place)
void TrimInPlace(std::string& s);
std::string Trim(std::string s);

uint64_t HashUsr(const std::string& s);
uint64_t HashUsr(const char* s);
uint64_t HashUsr(const char* s, size_t n);

// Returns true if |value| starts/ends with |start| or |ending|.
bool StartsWith(const std::string& value, const std::string& start);
bool EndsWith(const std::string& value, const std::string& ending);
bool AnyStartsWith(const std::vector<std::string>& values,
                   const std::string& start);
bool StartsWithAny(const std::string& value,
                   const std::vector<std::string>& startings);
bool EndsWithAny(const std::string& value,
                 const std::vector<std::string>& endings);
bool FindAnyPartial(const std::string& value,
                    const std::vector<std::string>& values);
// Returns the dirname of |path|, i.e. "foo/bar.cc" => "foo", "foo" => ".",
// "/foo" => "/".
std::string GetDirName(std::string path);
// Returns the basename of |path|, ie, "foo/bar.cc" => "bar.cc".
std::string GetBaseName(const std::string& path);
// Returns |path| without the filetype, ie, "foo/bar.cc" => "foo/bar".
std::string StripFileType(const std::string& path);

std::string ReplaceAll(const std::string& source,
                       const std::string& from,
                       const std::string& to);

std::vector<std::string> SplitString(const std::string& str,
                                     const std::string& delimiter);

std::string LowerPathIfCaseInsensitive(const std::string& path);

template <typename TValues, typename TMap>
std::string StringJoinMap(const TValues& values,
                          const TMap& map,
                          const std::string& sep = ", ") {
  std::string result;
  bool first = true;
  for (auto& entry : values) {
    if (!first)
      result += sep;
    first = false;
    result += map(entry);
  }
  return result;
}

template <typename TValues>
std::string StringJoin(const TValues& values, const std::string& sep = ", ") {
  return StringJoinMap(values, [](const std::string& entry) { return entry; },
                       sep);
}

template <typename TCollection, typename TValue>
bool ContainsValue(const TCollection& collection, const TValue& value) {
  return collection.find(value) != collection.end();
}

// Finds all files in the given folder. This is recursive.
std::vector<std::string> GetFilesInFolder(std::string folder,
                                          bool recursive,
                                          bool add_folder_to_path);
void GetFilesInFolder(std::string folder,
                      bool recursive,
                      bool add_folder_to_path,
                      const std::function<void(const std::string&)>& handler);

// Ensures that |path| ends in a slash.
void EnsureEndsInSlash(std::string& path);

// Converts a file path to one that can be used as filename.
// e.g. foo/bar.c => foo_bar.c
std::string EscapeFileName(std::string path);

// FIXME: Move ReadContent into ICacheManager?
bool FileExists(const std::string& filename);
optional<std::string> ReadContent(const std::string& filename);
std::vector<std::string> ReadLinesWithEnding(std::string filename);
std::vector<std::string> ToLines(const std::string& content,
                                 bool trim_whitespace);

struct TextReplacer {
  struct Replacement {
    std::string from;
    std::string to;
  };

  std::vector<Replacement> replacements;

  std::string Apply(const std::string& content);
};

void WriteToFile(const std::string& filename, const std::string& content);

template <typename T>
void AddRange(std::vector<T>* dest, const std::vector<T>& to_add) {
  dest->insert(dest->end(), to_add.begin(), to_add.end());
}

template <typename T>
void AddRange(std::vector<T>* dest, std::vector<T>&& to_add) {
  dest->insert(dest->end(), std::make_move_iterator(to_add.begin()),
               std::make_move_iterator(to_add.end()));
}

// http://stackoverflow.com/a/38140932
//
//  struct SomeHashKey {
//    std::string key1;
//    std::string key2;
//    bool key3;
//  };
//  MAKE_HASHABLE(SomeHashKey, t.key1, t.key2, t.key3)

inline void hash_combine(std::size_t& seed) {}

template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  hash_combine(seed, rest...);
}

#define MAKE_HASHABLE(type, ...)                  \
  namespace std {                                 \
  template <>                                     \
  struct hash<type> {                             \
    std::size_t operator()(const type& t) const { \
      std::size_t ret = 0;                        \
      hash_combine(ret, __VA_ARGS__);             \
      return ret;                                 \
    }                                             \
  };                                              \
  }

#define MAKE_ENUM_HASHABLE(type)                  \
  namespace std {                                 \
  template <>                                     \
  struct hash<type> {                             \
    std::size_t operator()(const type& t) const { \
      return hash<int>()(static_cast<int>(t));    \
    }                                             \
  };                                              \
  }

float GetProcessMemoryUsedInMb();

std::string FormatMicroseconds(long long microseconds);

std::string GetDefaultResourceDirectory();

// Makes sure all newlines in |output| are in \r\n format.
std::string UpdateToRnNewlines(std::string output);
