// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <optional>
#include <string_view>

#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace llvm {
class StringRef;
}

namespace ccls {
struct Matcher {
  struct Impl;
  std::unique_ptr<Impl> impl;
  std::string pattern;

  Matcher(const std::string &pattern); // throw
  Matcher(Matcher&&) = default;
  ~Matcher();
  bool Matches(const std::string &text) const;
};

struct GroupMatch {
  std::vector<Matcher> whitelist, blacklist;

  GroupMatch(const std::vector<std::string> &whitelist,
             const std::vector<std::string> &blacklist);
  bool Matches(const std::string &text,
               std::string *blacklist_pattern = nullptr) const;
};

uint64_t HashUsr(llvm::StringRef s);

std::string LowerPathIfInsensitive(const std::string &path);

// Ensures that |path| ends in a slash.
void EnsureEndsInSlash(std::string &path);

// Converts a file path to one that can be used as filename.
// e.g. foo/bar.c => foo_bar.c
std::string EscapeFileName(std::string path);

std::string ResolveIfRelative(const std::string &directory,
                              const std::string &path);
std::string RealPath(const std::string &path);
bool NormalizeFolder(std::string &path);

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

// Like std::optional, but the stored data is responsible for containing the
// empty state. T should define a function `bool T::Valid()`.
template <typename T> class Maybe {
  T storage;

public:
  constexpr Maybe() = default;
  Maybe(const Maybe &) = default;
  Maybe(std::nullopt_t) {}
  Maybe(const T &x) : storage(x) {}
  Maybe(T &&x) : storage(std::forward<T>(x)) {}

  Maybe &operator=(const Maybe &) = default;
  Maybe &operator=(const T &x) {
    storage = x;
    return *this;
  }

  const T *operator->() const { return &storage; }
  T *operator->() { return &storage; }
  const T &operator*() const { return storage; }
  T &operator*() { return storage; }

  bool Valid() const { return storage.Valid(); }
  explicit operator bool() const { return Valid(); }
  operator std::optional<T>() const {
    if (Valid())
      return storage;
    return std::nullopt;
  }

  void operator=(std::optional<T> &&o) { storage = o ? *o : T(); }

  // Does not test if has_value()
  bool operator==(const Maybe &o) const { return storage == o.storage; }
  bool operator!=(const Maybe &o) const { return !(*this == o); }
};

template <typename T> struct Vec {
  std::unique_ptr<T[]> a;
  int s = 0;
#if !(__clang__ || __GNUC__ > 7 || __GNUC__ == 7 && __GNUC_MINOR__ >= 4) || defined(_WIN32)
  // Work around a bug in GCC<7.4 that optional<IndexUpdate> would not be
  // construtible.
  Vec() = default;
  Vec(const Vec &o) : a(std::make_unique<T[]>(o.s)), s(o.s) {
    std::copy(o.a.get(), o.a.get() + o.s, a.get());
  }
  Vec(Vec &&) = default;
  Vec &operator=(Vec &&) = default;
  Vec(std::unique_ptr<T[]> a, int s) : a(std::move(a)), s(s) {}
#endif
  const T *begin() const { return a.get(); }
  T *begin() { return a.get(); }
  const T *end() const { return a.get() + s; }
  T *end() { return a.get() + s; }
  int size() const { return s; }
  const T &operator[](size_t i) const { return a.get()[i]; }
  T &operator[](size_t i) { return a.get()[i]; }
};
} // namespace ccls
