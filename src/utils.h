#pragma once

#include <optional>
#include <string_view>

#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

void TrimInPlace(std::string& s);
std::string Trim(std::string s);

uint64_t HashUsr(std::string_view s);

// Returns true if |value| starts/ends with |start| or |ending|.
bool StartsWith(std::string_view value, std::string_view start);
bool EndsWith(std::string_view value, std::string_view ending);
bool AnyStartsWith(const std::vector<std::string>& xs, std::string_view prefix);
bool StartsWithAny(std::string_view s, const std::vector<std::string>& ps);
bool EndsWithAny(std::string_view s, const std::vector<std::string>& ss);
bool FindAnyPartial(const std::string& value,
                    const std::vector<std::string>& values);

std::vector<std::string> SplitString(const std::string& str,
                                     const std::string& delimiter);

std::string LowerPathIfInsensitive(const std::string& path);

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

// Ensures that |path| ends in a slash.
void EnsureEndsInSlash(std::string& path);

// Converts a file path to one that can be used as filename.
// e.g. foo/bar.c => foo_bar.c
std::string EscapeFileName(std::string path);

std::optional<std::string> ReadContent(const std::string& filename);

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

std::string GetDefaultResourceDirectory();
