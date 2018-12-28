/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#pragma once

#include "utils.hh"

#include <llvm/Support/Compiler.h>

#include <macro_map.h>

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

class Reader {
public:
  virtual ~Reader();
  virtual SerializeFormat Format() const = 0;

  virtual bool IsBool() = 0;
  virtual bool IsNull() = 0;
  virtual bool IsInt() = 0;
  virtual bool IsInt64() = 0;
  virtual bool IsUInt64() = 0;
  virtual bool IsDouble() = 0;
  virtual bool IsString() = 0;

  virtual void GetNull() = 0;
  virtual bool GetBool() = 0;
  virtual uint8_t GetUInt8() = 0;
  virtual int GetInt() = 0;
  virtual uint32_t GetUInt32() = 0;
  virtual int64_t GetInt64() = 0;
  virtual uint64_t GetUInt64() = 0;
  virtual double GetDouble() = 0;
  virtual const char *GetString() = 0;

  virtual bool HasMember(const char *x) = 0;
  virtual std::unique_ptr<Reader> operator[](const char *x) = 0;

  virtual void IterArray(std::function<void(Reader &)> fn) = 0;
  virtual void Member(const char *name, std::function<void()> fn) = 0;
};

class Writer {
public:
  virtual ~Writer();
  virtual SerializeFormat Format() const = 0;

  virtual void Null() = 0;
  virtual void Bool(bool x) = 0;
  virtual void Int(int x) = 0;
  virtual void Int64(int64_t x) = 0;
  virtual void UInt8(uint8_t x) = 0;
  virtual void UInt32(uint32_t x) = 0;
  virtual void UInt64(uint64_t x) = 0;
  virtual void Double(double x) = 0;
  virtual void String(const char *x) = 0;
  virtual void String(const char *x, size_t len) = 0;
  virtual void StartArray(size_t) = 0;
  virtual void EndArray() = 0;
  virtual void StartObject() = 0;
  virtual void EndObject() = 0;
  virtual void Key(const char *name) = 0;
};

struct IndexFile;

#define REFLECT_MEMBER_START() ReflectMemberStart(vis)
#define REFLECT_MEMBER_END() ReflectMemberEnd(vis);
#define REFLECT_MEMBER(name) ReflectMember(vis, #name, v.name)
#define REFLECT_MEMBER2(name, v) ReflectMember(vis, name, v)

#define MAKE_REFLECT_TYPE_PROXY(type_name)                                     \
  MAKE_REFLECT_TYPE_PROXY2(type_name, std::underlying_type_t<type_name>)
#define MAKE_REFLECT_TYPE_PROXY2(type, as_type)                                \
  LLVM_ATTRIBUTE_UNUSED inline void Reflect(Reader &vis, type &v) {    \
    as_type value0;                                                            \
    ::ccls::Reflect(vis, value0);                                          \
    v = static_cast<type>(value0);                                         \
  }                                                                            \
  LLVM_ATTRIBUTE_UNUSED inline void Reflect(Writer &vis, type &v) {    \
    auto value0 = static_cast<as_type>(v);                                 \
    ::ccls::Reflect(vis, value0);                                          \
  }

#define _MAPPABLE_REFLECT_MEMBER(name) REFLECT_MEMBER(name);

#define MAKE_REFLECT_EMPTY_STRUCT(type, ...)                                   \
  template <typename TVisitor> void Reflect(TVisitor &vis, type &v) {  \
    REFLECT_MEMBER_START();                                                    \
    REFLECT_MEMBER_END();                                                      \
  }

#define MAKE_REFLECT_STRUCT(type, ...)                                         \
  template <typename TVisitor> void Reflect(TVisitor &vis, type &v) {  \
    REFLECT_MEMBER_START();                                                    \
    MACRO_MAP(_MAPPABLE_REFLECT_MEMBER, __VA_ARGS__)                           \
    REFLECT_MEMBER_END();                                                      \
  }

// clang-format off
// Config has many fields, we need to support at least its number of fields.
#define NUM_VA_ARGS_IMPL(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,N,...) N
#define NUM_VA_ARGS(...) NUM_VA_ARGS_IMPL(__VA_ARGS__,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1)
// clang-format on

#define _MAPPABLE_REFLECT_ARRAY(name) Reflect(vis, v.name);

// Reflects the struct so it is serialized as an array instead of an object.
// This currently only supports writers.
#define MAKE_REFLECT_STRUCT_WRITER_AS_ARRAY(type, ...)                         \
  inline void Reflect(Writer &vis, type &v) {                                  \
    vis.StartArray(NUM_VA_ARGS(__VA_ARGS__));                                  \
    MACRO_MAP(_MAPPABLE_REFLECT_ARRAY, __VA_ARGS__)                            \
    vis.EndArray();                                                            \
  }

//// Elementary types

void Reflect(Reader &vis, uint8_t &v);
void Reflect(Writer &vis, uint8_t &v);

void Reflect(Reader &vis, short &v);
void Reflect(Writer &vis, short &v);

void Reflect(Reader &vis, unsigned short &v);
void Reflect(Writer &vis, unsigned short &v);

void Reflect(Reader &vis, int &v);
void Reflect(Writer &vis, int &v);

void Reflect(Reader &vis, unsigned &v);
void Reflect(Writer &vis, unsigned &v);

void Reflect(Reader &vis, long &v);
void Reflect(Writer &vis, long &v);

void Reflect(Reader &vis, unsigned long &v);
void Reflect(Writer &vis, unsigned long &v);

void Reflect(Reader &vis, long long &v);
void Reflect(Writer &vis, long long &v);

void Reflect(Reader &vis, unsigned long long &v);
void Reflect(Writer &vis, unsigned long long &v);

void Reflect(Reader &vis, double &v);
void Reflect(Writer &vis, double &v);

void Reflect(Reader &vis, bool &v);
void Reflect(Writer &vis, bool &v);

void Reflect(Reader &vis, std::string &v);
void Reflect(Writer &vis, std::string &v);

void Reflect(Reader &vis, std::string_view &v);
void Reflect(Writer &vis, std::string_view &v);

void Reflect(Reader &vis, const char *&v);
void Reflect(Writer &vis, const char *&v);

void Reflect(Reader &vis, JsonNull &v);
void Reflect(Writer &vis, JsonNull &v);

void Reflect(Reader &vis, SerializeFormat &v);
void Reflect(Writer &vis, SerializeFormat &v);

//// Type constructors

// ReflectMember std::optional<T> is used to represent TypeScript optional
// properties (in `key: value` context). Reflect std::optional<T> is used for a
// different purpose, whether an object is nullable (possibly in `value`
// context).
template <typename T> void Reflect(Reader &vis, std::optional<T> &v) {
  if (vis.IsNull()) {
    vis.GetNull();
    return;
  }
  T val;
  Reflect(vis, val);
  v = std::move(val);
}
template <typename T> void Reflect(Writer &vis, std::optional<T> &v) {
  if (v) {
    if (vis.Format() != SerializeFormat::Json)
      vis.UInt8(1);
    Reflect(vis, *v);
  } else
    vis.Null();
}

// The same as std::optional
template <typename T> void Reflect(Reader &vis, Maybe<T> &v) {
  if (vis.IsNull()) {
    vis.GetNull();
    return;
  }
  T val;
  Reflect(vis, val);
  v = std::move(val);
}
template <typename T> void Reflect(Writer &vis, Maybe<T> &v) {
  if (v) {
    if (vis.Format() != SerializeFormat::Json)
      vis.UInt8(1);
    Reflect(vis, *v);
  } else
    vis.Null();
}

template <typename T>
void ReflectMember(Writer &vis, const char *name, std::optional<T> &v) {
  // For TypeScript std::optional property key?: value in the spec,
  // We omit both key and value if value is std::nullopt (null) for JsonWriter
  // to reduce output. But keep it for other serialization formats.
  if (v || vis.Format() != SerializeFormat::Json) {
    vis.Key(name);
    Reflect(vis, v);
  }
}

// The same as std::optional
template <typename T>
void ReflectMember(Writer &vis, const char *name, Maybe<T> &v) {
  if (v.Valid() || vis.Format() != SerializeFormat::Json) {
    vis.Key(name);
    Reflect(vis, v);
  }
}

template <typename L, typename R>
void Reflect(Reader &vis, std::pair<L, R> &v) {
  vis.Member("L", [&]() { Reflect(vis, v.first); });
  vis.Member("R", [&]() { Reflect(vis, v.second); });
}
template <typename L, typename R>
void Reflect(Writer &vis, std::pair<L, R> &v) {
  vis.StartObject();
  ReflectMember(vis, "L", v.first);
  ReflectMember(vis, "R", v.second);
  vis.EndObject();
}

// std::vector
template <typename T> void Reflect(Reader &vis, std::vector<T> &vs) {
  vis.IterArray([&](Reader &entry) {
    T entry_value;
    Reflect(entry, entry_value);
    vs.push_back(std::move(entry_value));
  });
}
template <typename T> void Reflect(Writer &vis, std::vector<T> &vs) {
  vis.StartArray(vs.size());
  for (auto &v : vs)
    Reflect(vis, v);
  vis.EndArray();
}

// ReflectMember

inline bool ReflectMemberStart(Reader &vis) { return false; }
inline bool ReflectMemberStart(Writer &vis) {
  vis.StartObject();
  return true;
}

inline void ReflectMemberEnd(Reader &vis) {}
inline void ReflectMemberEnd(Writer &vis) { vis.EndObject(); }

template <typename T> void ReflectMember(Reader &vis, const char *name, T &v) {
  vis.Member(name, [&]() { Reflect(vis, v); });
}
template <typename T> void ReflectMember(Writer &vis, const char *name, T &v) {
  vis.Key(name);
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
