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
}

namespace ccls {
enum class SerializeFormat { Binary, Json };

struct JsonNull {};

struct JsonReader {
  rapidjson::Value *m;
  std::vector<const char *> path_;

  JsonReader(rapidjson::Value *m) : m(m) {}
  void StartObject() {}
  void EndObject() {}
  void IterArray(std::function<void()> fn);
  void Member(const char *name, std::function<void()> fn);
  bool IsNull();
  std::string GetString();
  std::string GetPath() const;
};

struct JsonWriter {
  using W =
      rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<char>,
                        rapidjson::UTF8<char>, rapidjson::CrtAllocator, 0>;

  W *m;

  JsonWriter(W *m) : m(m) {}
  void StartArray();
  void EndArray();
  void StartObject();
  void EndObject();
  void Key(const char *name);
  void Null();
  void Int(int v);
  void String(const char *s);
  void String(const char *s, size_t len);
};

struct BinaryReader {
  const char *p_;

  BinaryReader(std::string_view buf) : p_(buf.data()) {}
  template <typename T> T Get() {
    T ret;
    memcpy(&ret, p_, sizeof(T));
    p_ += sizeof(T);
    return ret;
  }
  uint64_t VarUInt() {
    auto x = *reinterpret_cast<const uint8_t *>(p_++);
    if (x < 253)
      return x;
    if (x == 253)
      return Get<uint16_t>();
    if (x == 254)
      return Get<uint32_t>();
    return Get<uint64_t>();
  }
  int64_t VarInt() {
    uint64_t x = VarUInt();
    return int64_t(x >> 1 ^ -(x & 1));
  }
  const char *GetString() {
    const char *ret = p_;
    while (*p_)
      p_++;
    p_++;
    return ret;
  }
};

struct BinaryWriter {
  std::string buf_;

  template <typename T> void Pack(T x) {
    auto i = buf_.size();
    buf_.resize(i + sizeof(x));
    memcpy(buf_.data() + i, &x, sizeof(x));
  }

  void VarUInt(uint64_t n) {
    if (n < 253)
      Pack<uint8_t>(n);
    else if (n < 65536) {
      Pack<uint8_t>(253);
      Pack<uint16_t>(n);
    } else if (n < 4294967296) {
      Pack<uint8_t>(254);
      Pack<uint32_t>(n);
    } else {
      Pack<uint8_t>(255);
      Pack<uint64_t>(n);
    }
  }
  void VarInt(int64_t n) { VarUInt(uint64_t(n) << 1 ^ n >> 63); }
  std::string Take() { return std::move(buf_); }

  void String(const char *x) { String(x, strlen(x)); }
  void String(const char *x, size_t len) {
    auto i = buf_.size();
    buf_.resize(i + len + 1);
    memcpy(buf_.data() + i, x, len);
  }
};

struct IndexFile;

#define REFLECT_MEMBER(name) ReflectMember(vis, #name, v.name)
#define REFLECT_MEMBER2(name, v) ReflectMember(vis, name, v)

#define REFLECT_UNDERLYING(T)                                                  \
  LLVM_ATTRIBUTE_UNUSED inline void Reflect(JsonReader &vis, T &v) {           \
    std::underlying_type_t<T> v0;                                              \
    ::ccls::Reflect(vis, v0);                                                  \
    v = static_cast<T>(v0);                                                    \
  }                                                                            \
  LLVM_ATTRIBUTE_UNUSED inline void Reflect(JsonWriter &vis, T &v) {           \
    auto v0 = static_cast<std::underlying_type_t<T>>(v);                       \
    ::ccls::Reflect(vis, v0);                                                  \
  }

#define REFLECT_UNDERLYING_B(T)                                                \
  REFLECT_UNDERLYING(T)                                                        \
  LLVM_ATTRIBUTE_UNUSED inline void Reflect(BinaryReader &vis, T &v) {         \
    std::underlying_type_t<T> v0;                                              \
    ::ccls::Reflect(vis, v0);                                                  \
    v = static_cast<T>(v0);                                                    \
  }                                                                            \
  LLVM_ATTRIBUTE_UNUSED inline void Reflect(BinaryWriter &vis, T &v) {         \
    auto v0 = static_cast<std::underlying_type_t<T>>(v);                       \
    ::ccls::Reflect(vis, v0);                                                  \
  }

#define _MAPPABLE_REFLECT_MEMBER(name) REFLECT_MEMBER(name);

#define REFLECT_STRUCT(type, ...)                                              \
  template <typename Vis> void Reflect(Vis &vis, type &v) {                    \
    ReflectMemberStart(vis);                                                   \
    MACRO_MAP(_MAPPABLE_REFLECT_MEMBER, __VA_ARGS__)                           \
    ReflectMemberEnd(vis);                                                     \
  }

#define _MAPPABLE_REFLECT_ARRAY(name) Reflect(vis, v.name);

void Reflect(JsonReader &vis, bool &v);
void Reflect(JsonReader &vis, unsigned char &v);
void Reflect(JsonReader &vis, short &v);
void Reflect(JsonReader &vis, unsigned short &v);
void Reflect(JsonReader &vis, int &v);
void Reflect(JsonReader &vis, unsigned &v);
void Reflect(JsonReader &vis, long &v);
void Reflect(JsonReader &vis, unsigned long &v);
void Reflect(JsonReader &vis, long long &v);
void Reflect(JsonReader &vis, unsigned long long &v);
void Reflect(JsonReader &vis, double &v);
void Reflect(JsonReader &vis, const char *&v);
void Reflect(JsonReader &vis, std::string &v);

void Reflect(JsonWriter &vis, bool &v);
void Reflect(JsonWriter &vis, unsigned char &v);
void Reflect(JsonWriter &vis, short &v);
void Reflect(JsonWriter &vis, unsigned short &v);
void Reflect(JsonWriter &vis, int &v);
void Reflect(JsonWriter &vis, unsigned &v);
void Reflect(JsonWriter &vis, long &v);
void Reflect(JsonWriter &vis, unsigned long &v);
void Reflect(JsonWriter &vis, long long &v);
void Reflect(JsonWriter &vis, unsigned long long &v);
void Reflect(JsonWriter &vis, double &v);
void Reflect(JsonWriter &vis, const char *&v);
void Reflect(JsonWriter &vis, std::string &v);

void Reflect(BinaryReader &vis, bool &v);
void Reflect(BinaryReader &vis, unsigned char &v);
void Reflect(BinaryReader &vis, short &v);
void Reflect(BinaryReader &vis, unsigned short &v);
void Reflect(BinaryReader &vis, int &v);
void Reflect(BinaryReader &vis, unsigned &v);
void Reflect(BinaryReader &vis, long &v);
void Reflect(BinaryReader &vis, unsigned long &v);
void Reflect(BinaryReader &vis, long long &v);
void Reflect(BinaryReader &vis, unsigned long long &v);
void Reflect(BinaryReader &vis, double &v);
void Reflect(BinaryReader &vis, const char *&v);
void Reflect(BinaryReader &vis, std::string &v);

void Reflect(BinaryWriter &vis, bool &v);
void Reflect(BinaryWriter &vis, unsigned char &v);
void Reflect(BinaryWriter &vis, short &v);
void Reflect(BinaryWriter &vis, unsigned short &v);
void Reflect(BinaryWriter &vis, int &v);
void Reflect(BinaryWriter &vis, unsigned &v);
void Reflect(BinaryWriter &vis, long &v);
void Reflect(BinaryWriter &vis, unsigned long &v);
void Reflect(BinaryWriter &vis, long long &v);
void Reflect(BinaryWriter &vis, unsigned long long &v);
void Reflect(BinaryWriter &vis, double &v);
void Reflect(BinaryWriter &vis, const char *&v);
void Reflect(BinaryWriter &vis, std::string &v);

void Reflect(JsonReader &vis, JsonNull &v);
void Reflect(JsonWriter &vis, JsonNull &v);

void Reflect(JsonReader &vis, SerializeFormat &v);
void Reflect(JsonWriter &vis, SerializeFormat &v);

void Reflect(JsonWriter &vis, std::string_view &v);

//// Type constructors

// ReflectMember std::optional<T> is used to represent TypeScript optional
// properties (in `key: value` context). Reflect std::optional<T> is used for a
// different purpose, whether an object is nullable (possibly in `value`
// context).
template <typename T> void Reflect(JsonReader &vis, std::optional<T> &v) {
  if (!vis.IsNull()) {
    v.emplace();
    Reflect(vis, *v);
  }
}
template <typename T> void Reflect(JsonWriter &vis, std::optional<T> &v) {
  if (v)
    Reflect(vis, *v);
  else
    vis.Null();
}
template <typename T> void Reflect(BinaryReader &vis, std::optional<T> &v) {
  if (*vis.p_++) {
    v.emplace();
    Reflect(vis, *v);
  }
}
template <typename T> void Reflect(BinaryWriter &vis, std::optional<T> &v) {
  if (v) {
    vis.Pack<unsigned char>(1);
    Reflect(vis, *v);
  } else {
    vis.Pack<unsigned char>(0);
  }
}

// The same as std::optional
template <typename T> void Reflect(JsonReader &vis, Maybe<T> &v) {
  if (!vis.IsNull())
    Reflect(vis, *v);
}
template <typename T> void Reflect(JsonWriter &vis, Maybe<T> &v) {
  if (v)
    Reflect(vis, *v);
  else
    vis.Null();
}
template <typename T> void Reflect(BinaryReader &vis, Maybe<T> &v) {
  if (*vis.p_++)
    Reflect(vis, *v);
}
template <typename T> void Reflect(BinaryWriter &vis, Maybe<T> &v) {
  if (v) {
    vis.Pack<unsigned char>(1);
    Reflect(vis, *v);
  } else {
    vis.Pack<unsigned char>(0);
  }
}

template <typename T>
void ReflectMember(JsonWriter &vis, const char *name, std::optional<T> &v) {
  // For TypeScript std::optional property key?: value in the spec,
  // We omit both key and value if value is std::nullopt (null) for JsonWriter
  // to reduce output. But keep it for other serialization formats.
  if (v) {
    vis.Key(name);
    Reflect(vis, *v);
  }
}
template <typename T>
void ReflectMember(BinaryWriter &vis, const char *, std::optional<T> &v) {
  Reflect(vis, v);
}

// The same as std::optional
template <typename T>
void ReflectMember(JsonWriter &vis, const char *name, Maybe<T> &v) {
  if (v.Valid()) {
    vis.Key(name);
    Reflect(vis, v);
  }
}
template <typename T>
void ReflectMember(BinaryWriter &vis, const char *, Maybe<T> &v) {
  Reflect(vis, v);
}

template <typename L, typename R>
void Reflect(JsonReader &vis, std::pair<L, R> &v) {
  vis.Member("L", [&]() { Reflect(vis, v.first); });
  vis.Member("R", [&]() { Reflect(vis, v.second); });
}
template <typename L, typename R>
void Reflect(JsonWriter &vis, std::pair<L, R> &v) {
  vis.StartObject();
  ReflectMember(vis, "L", v.first);
  ReflectMember(vis, "R", v.second);
  vis.EndObject();
}
template <typename L, typename R>
void Reflect(BinaryReader &vis, std::pair<L, R> &v) {
  Reflect(vis, v.first);
  Reflect(vis, v.second);
}
template <typename L, typename R>
void Reflect(BinaryWriter &vis, std::pair<L, R> &v) {
  Reflect(vis, v.first);
  Reflect(vis, v.second);
}

// std::vector
template <typename T> void Reflect(JsonReader &vis, std::vector<T> &v) {
  vis.IterArray([&]() {
    v.emplace_back();
    Reflect(vis, v.back());
  });
}
template <typename T> void Reflect(JsonWriter &vis, std::vector<T> &v) {
  vis.StartArray();
  for (auto &it : v)
    Reflect(vis, it);
  vis.EndArray();
}
template <typename T> void Reflect(BinaryReader &vis, std::vector<T> &v) {
  for (auto n = vis.VarUInt(); n; n--) {
    v.emplace_back();
    Reflect(vis, v.back());
  }
}
template <typename T> void Reflect(BinaryWriter &vis, std::vector<T> &v) {
  vis.VarUInt(v.size());
  for (auto &it : v)
    Reflect(vis, it);
}

// ReflectMember

void ReflectMemberStart(JsonReader &);
template <typename T> void ReflectMemberStart(T &) {}
inline void ReflectMemberStart(JsonWriter &vis) { vis.StartObject(); }

template <typename T> void ReflectMemberEnd(T &) {}
inline void ReflectMemberEnd(JsonWriter &vis) { vis.EndObject(); }

template <typename T> void ReflectMember(JsonReader &vis, const char *name, T &v) {
  vis.Member(name, [&]() { Reflect(vis, v); });
}
template <typename T> void ReflectMember(JsonWriter &vis, const char *name, T &v) {
  vis.Key(name);
  Reflect(vis, v);
}
template <typename T> void ReflectMember(BinaryReader &vis, const char *, T &v) {
  Reflect(vis, v);
}
template <typename T> void ReflectMember(BinaryWriter &vis, const char *, T &v) {
  Reflect(vis, v);
}

// API

const char *Intern(llvm::StringRef str);
llvm::CachedHashStringRef InternH(llvm::StringRef str);
std::string Serialize(SerializeFormat format, IndexFile &file);
std::unique_ptr<IndexFile>
Deserialize(SerializeFormat format, const std::string &path,
            const std::string &serialized_index_content,
            const std::string &file_content,
            std::optional<int> expected_version);
} // namespace ccls
