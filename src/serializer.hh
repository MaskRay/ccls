// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "utils.hh"

#include <llvm/Support/Compiler.h>

#include <macro_map.h>
#include <rapidjson/fwd.h>

#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace llvm {
class CachedHashStringRef;
class StringRef;
template <typename Fn> class function_ref;
} // namespace llvm

namespace ccls {
enum class SerializeFormat { Binary, Json };

struct JsonNull {};

struct JsonReader {
  rapidjson::Value *m;
  std::vector<const char *> path_;

  JsonReader(rapidjson::Value *m) : m(m) {}
  void startObject() {}
  void endObject() {}
  void iterArray(llvm::function_ref<void()> fn);
  void member(const char *name, llvm::function_ref<void()> fn);
  bool isNull();
  std::string getString();
  std::string getPath() const;
};

struct JsonWriter {
  using W =
      rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<char>,
                        rapidjson::UTF8<char>, rapidjson::CrtAllocator, 0>;

  W *m;

  JsonWriter(W *m) : m(m) {}
  void startArray();
  void endArray();
  void startObject();
  void endObject();
  void key(const char *name);
  void null_();
  void int64(int64_t v);
  void string(const char *s);
  void string(const char *s, size_t len);
};

struct BinaryReader {
  const char *p_;

  BinaryReader(std::string_view buf) : p_(buf.data()) {}
  template <typename T> T get() {
    T ret;
    memcpy(&ret, p_, sizeof(T));
    p_ += sizeof(T);
    return ret;
  }
  uint64_t varUInt() {
    auto x = *reinterpret_cast<const uint8_t *>(p_++);
    if (x < 253)
      return x;
    if (x == 253)
      return get<uint16_t>();
    if (x == 254)
      return get<uint32_t>();
    return get<uint64_t>();
  }
  int64_t varInt() {
    uint64_t x = varUInt();
    return int64_t(x >> 1 ^ -(x & 1));
  }
  const char *getString() {
    const char *ret = p_;
    while (*p_)
      p_++;
    p_++;
    return ret;
  }
};

struct BinaryWriter {
  std::string buf_;

  template <typename T> void pack(T x) {
    auto i = buf_.size();
    buf_.resize(i + sizeof(x));
    memcpy(buf_.data() + i, &x, sizeof(x));
  }

  void varUInt(uint64_t n) {
    if (n < 253)
      pack<uint8_t>(n);
    else if (n < 65536) {
      pack<uint8_t>(253);
      pack<uint16_t>(n);
    } else if (n < 4294967296) {
      pack<uint8_t>(254);
      pack<uint32_t>(n);
    } else {
      pack<uint8_t>(255);
      pack<uint64_t>(n);
    }
  }
  void varInt(int64_t n) { varUInt(uint64_t(n) << 1 ^ n >> 63); }
  std::string take() { return std::move(buf_); }

  void string(const char *x) { string(x, strlen(x)); }
  void string(const char *x, size_t len) {
    auto i = buf_.size();
    buf_.resize(i + len + 1);
    memcpy(buf_.data() + i, x, len);
  }
};

struct IndexFile;

#define REFLECT_MEMBER(name) reflectMember(vis, #name, v.name)
#define REFLECT_MEMBER2(name, v) reflectMember(vis, name, v)

#define REFLECT_UNDERLYING(T)                                                  \
  LLVM_ATTRIBUTE_UNUSED inline void reflect(JsonReader &vis, T &v) {           \
    std::underlying_type_t<T> v0;                                              \
    ::ccls::reflect(vis, v0);                                                  \
    v = static_cast<T>(v0);                                                    \
  }                                                                            \
  LLVM_ATTRIBUTE_UNUSED inline void reflect(JsonWriter &vis, T &v) {           \
    auto v0 = static_cast<std::underlying_type_t<T>>(v);                       \
    ::ccls::reflect(vis, v0);                                                  \
  }

#define REFLECT_UNDERLYING_B(T)                                                \
  REFLECT_UNDERLYING(T)                                                        \
  LLVM_ATTRIBUTE_UNUSED inline void reflect(BinaryReader &vis, T &v) {         \
    std::underlying_type_t<T> v0;                                              \
    ::ccls::reflect(vis, v0);                                                  \
    v = static_cast<T>(v0);                                                    \
  }                                                                            \
  LLVM_ATTRIBUTE_UNUSED inline void reflect(BinaryWriter &vis, T &v) {         \
    auto v0 = static_cast<std::underlying_type_t<T>>(v);                       \
    ::ccls::reflect(vis, v0);                                                  \
  }

#define _MAPPABLE_REFLECT_MEMBER(name) REFLECT_MEMBER(name);

#define REFLECT_STRUCT(type, ...)                                              \
  template <typename Vis> void reflect(Vis &vis, type &v) {                    \
    reflectMemberStart(vis);                                                   \
    MACRO_MAP(_MAPPABLE_REFLECT_MEMBER, __VA_ARGS__)                           \
    reflectMemberEnd(vis);                                                     \
  }

#define _MAPPABLE_REFLECT_ARRAY(name) reflect(vis, v.name);

void reflect(JsonReader &vis, bool &v);
void reflect(JsonReader &vis, unsigned char &v);
void reflect(JsonReader &vis, short &v);
void reflect(JsonReader &vis, unsigned short &v);
void reflect(JsonReader &vis, int &v);
void reflect(JsonReader &vis, unsigned &v);
void reflect(JsonReader &vis, long &v);
void reflect(JsonReader &vis, unsigned long &v);
void reflect(JsonReader &vis, long long &v);
void reflect(JsonReader &vis, unsigned long long &v);
void reflect(JsonReader &vis, double &v);
void reflect(JsonReader &vis, const char *&v);
void reflect(JsonReader &vis, std::string &v);

void reflect(JsonWriter &vis, bool &v);
void reflect(JsonWriter &vis, unsigned char &v);
void reflect(JsonWriter &vis, short &v);
void reflect(JsonWriter &vis, unsigned short &v);
void reflect(JsonWriter &vis, int &v);
void reflect(JsonWriter &vis, unsigned &v);
void reflect(JsonWriter &vis, long &v);
void reflect(JsonWriter &vis, unsigned long &v);
void reflect(JsonWriter &vis, long long &v);
void reflect(JsonWriter &vis, unsigned long long &v);
void reflect(JsonWriter &vis, double &v);
void reflect(JsonWriter &vis, const char *&v);
void reflect(JsonWriter &vis, std::string &v);

void reflect(BinaryReader &vis, bool &v);
void reflect(BinaryReader &vis, unsigned char &v);
void reflect(BinaryReader &vis, short &v);
void reflect(BinaryReader &vis, unsigned short &v);
void reflect(BinaryReader &vis, int &v);
void reflect(BinaryReader &vis, unsigned &v);
void reflect(BinaryReader &vis, long &v);
void reflect(BinaryReader &vis, unsigned long &v);
void reflect(BinaryReader &vis, long long &v);
void reflect(BinaryReader &vis, unsigned long long &v);
void reflect(BinaryReader &vis, double &v);
void reflect(BinaryReader &vis, const char *&v);
void reflect(BinaryReader &vis, std::string &v);

void reflect(BinaryWriter &vis, bool &v);
void reflect(BinaryWriter &vis, unsigned char &v);
void reflect(BinaryWriter &vis, short &v);
void reflect(BinaryWriter &vis, unsigned short &v);
void reflect(BinaryWriter &vis, int &v);
void reflect(BinaryWriter &vis, unsigned &v);
void reflect(BinaryWriter &vis, long &v);
void reflect(BinaryWriter &vis, unsigned long &v);
void reflect(BinaryWriter &vis, long long &v);
void reflect(BinaryWriter &vis, unsigned long long &v);
void reflect(BinaryWriter &vis, double &v);
void reflect(BinaryWriter &vis, const char *&v);
void reflect(BinaryWriter &vis, std::string &v);

void reflect(JsonReader &vis, JsonNull &v);
void reflect(JsonWriter &vis, JsonNull &v);

void reflect(JsonReader &vis, SerializeFormat &v);
void reflect(JsonWriter &vis, SerializeFormat &v);

void reflect(JsonWriter &vis, std::string_view &v);

//// Type constructors

// reflectMember std::optional<T> is used to represent TypeScript optional
// properties (in `key: value` context). reflect std::optional<T> is used for a
// different purpose, whether an object is nullable (possibly in `value`
// context).
template <typename T> void reflect(JsonReader &vis, std::optional<T> &v) {
  if (!vis.isNull()) {
    v.emplace();
    reflect(vis, *v);
  }
}
template <typename T> void reflect(JsonWriter &vis, std::optional<T> &v) {
  if (v)
    reflect(vis, *v);
  else
    vis.null_();
}
template <typename T> void reflect(BinaryReader &vis, std::optional<T> &v) {
  if (*vis.p_++) {
    v.emplace();
    reflect(vis, *v);
  }
}
template <typename T> void reflect(BinaryWriter &vis, std::optional<T> &v) {
  if (v) {
    vis.pack<unsigned char>(1);
    reflect(vis, *v);
  } else {
    vis.pack<unsigned char>(0);
  }
}

// The same as std::optional
template <typename T> void reflect(JsonReader &vis, Maybe<T> &v) {
  if (!vis.isNull())
    reflect(vis, *v);
}
template <typename T> void reflect(JsonWriter &vis, Maybe<T> &v) {
  if (v)
    reflect(vis, *v);
  else
    vis.null_();
}
template <typename T> void reflect(BinaryReader &vis, Maybe<T> &v) {
  if (*vis.p_++)
    reflect(vis, *v);
}
template <typename T> void reflect(BinaryWriter &vis, Maybe<T> &v) {
  if (v) {
    vis.pack<unsigned char>(1);
    reflect(vis, *v);
  } else {
    vis.pack<unsigned char>(0);
  }
}

template <typename T>
void reflectMember(JsonWriter &vis, const char *name, std::optional<T> &v) {
  // For TypeScript std::optional property key?: value in the spec,
  // We omit both key and value if value is std::nullopt (null) for JsonWriter
  // to reduce output. But keep it for other serialization formats.
  if (v) {
    vis.key(name);
    reflect(vis, *v);
  }
}
template <typename T>
void reflectMember(BinaryWriter &vis, const char *, std::optional<T> &v) {
  reflect(vis, v);
}

// The same as std::optional
template <typename T>
void reflectMember(JsonWriter &vis, const char *name, Maybe<T> &v) {
  if (v.valid()) {
    vis.key(name);
    reflect(vis, v);
  }
}
template <typename T>
void reflectMember(BinaryWriter &vis, const char *, Maybe<T> &v) {
  reflect(vis, v);
}

template <typename L, typename R>
void reflect(JsonReader &vis, std::pair<L, R> &v) {
  vis.member("L", [&]() { reflect(vis, v.first); });
  vis.member("R", [&]() { reflect(vis, v.second); });
}
template <typename L, typename R>
void reflect(JsonWriter &vis, std::pair<L, R> &v) {
  vis.startObject();
  reflectMember(vis, "L", v.first);
  reflectMember(vis, "R", v.second);
  vis.endObject();
}
template <typename L, typename R>
void reflect(BinaryReader &vis, std::pair<L, R> &v) {
  reflect(vis, v.first);
  reflect(vis, v.second);
}
template <typename L, typename R>
void reflect(BinaryWriter &vis, std::pair<L, R> &v) {
  reflect(vis, v.first);
  reflect(vis, v.second);
}

// std::vector
template <typename T> void reflect(JsonReader &vis, std::vector<T> &v) {
  vis.iterArray([&]() {
    v.emplace_back();
    reflect(vis, v.back());
  });
}
template <typename T> void reflect(JsonWriter &vis, std::vector<T> &v) {
  vis.startArray();
  for (auto &it : v)
    reflect(vis, it);
  vis.endArray();
}
template <typename T> void reflect(BinaryReader &vis, std::vector<T> &v) {
  for (auto n = vis.varUInt(); n; n--) {
    v.emplace_back();
    reflect(vis, v.back());
  }
}
template <typename T> void reflect(BinaryWriter &vis, std::vector<T> &v) {
  vis.varUInt(v.size());
  for (auto &it : v)
    reflect(vis, it);
}

// reflectMember

void reflectMemberStart(JsonReader &);
template <typename T> void reflectMemberStart(T &) {}
inline void reflectMemberStart(JsonWriter &vis) { vis.startObject(); }

template <typename T> void reflectMemberEnd(T &) {}
inline void reflectMemberEnd(JsonWriter &vis) { vis.endObject(); }

template <typename T>
void reflectMember(JsonReader &vis, const char *name, T &v) {
  vis.member(name, [&]() { reflect(vis, v); });
}
template <typename T>
void reflectMember(JsonWriter &vis, const char *name, T &v) {
  vis.key(name);
  reflect(vis, v);
}
template <typename T>
void reflectMember(BinaryReader &vis, const char *, T &v) {
  reflect(vis, v);
}
template <typename T>
void reflectMember(BinaryWriter &vis, const char *, T &v) {
  reflect(vis, v);
}

// API

const char *intern(llvm::StringRef str);
llvm::CachedHashStringRef internH(llvm::StringRef str);
std::string serialize(SerializeFormat format, IndexFile &file);
std::unique_ptr<IndexFile>
deserialize(SerializeFormat format, const std::string &path,
            const std::string &serialized_index_content,
            const std::string &file_content,
            std::optional<int> expected_version);
} // namespace ccls
