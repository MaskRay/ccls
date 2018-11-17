// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string_view>

#include <iterator>
#include <string>
#include <vector>

namespace llvm {
class StringRef;
}

namespace ccls {
uint64_t HashUsr(llvm::StringRef s);

std::string LowerPathIfInsensitive(const std::string &path);

// Ensures that |path| ends in a slash.
void EnsureEndsInSlash(std::string &path);

// Converts a file path to one that can be used as filename.
// e.g. foo/bar.c => foo_bar.c
std::string EscapeFileName(std::string path);

std::string ResolveIfRelative(const std::string &directory,
                              const std::string &path);

std::optional<int64_t> LastWriteTime(const std::string &path);
std::optional<std::string> ReadContent(const std::string &filename);
void WriteToFile(const std::string &filename, const std::string &content);

int ReverseSubseqMatch(std::string_view pat, std::string_view text,
                       int case_sensitivity);

// http://stackoverflow.com/a/38140932
//
//  struct SomeHashKey {
//    std::string key1;
//    std::string key2;
//    bool key3;
//  };
//  MAKE_HASHABLE(SomeHashKey, t.key1, t.key2, t.key3)

inline void hash_combine(std::size_t &seed) {}

template <typename T, typename... Rest>
inline void hash_combine(std::size_t &seed, const T &v, Rest... rest) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  hash_combine(seed, rest...);
}

#define MAKE_HASHABLE(type, ...)                                               \
  namespace std {                                                              \
  template <> struct hash<type> {                                              \
    std::size_t operator()(const type &t) const {                              \
      std::size_t ret = 0;                                                     \
      ccls::hash_combine(ret, __VA_ARGS__);                                          \
      return ret;                                                              \
    }                                                                          \
  };                                                                           \
  }

std::string GetDefaultResourceDirectory();
} // namespace ccls
