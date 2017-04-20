#pragma once

#include <macro_map.h>
#include <optional.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include <vector>
#include <string>


using std::experimental::optional;
using std::experimental::nullopt;

using Reader = rapidjson::GenericValue<rapidjson::UTF8<>>;
using Writer = rapidjson::Writer<rapidjson::StringBuffer>;
struct IndexedFile;

#define REFLECT_MEMBER_START() \
    if (!ReflectMemberStart(visitor, value)) return
#define REFLECT_MEMBER_START1(value) \
    if (!ReflectMemberStart(visitor, value)) return
#define REFLECT_MEMBER_END() \
    ReflectMemberEnd(visitor, value);
#define REFLECT_MEMBER_END1(value) \
    ReflectMemberEnd(visitor, value);
#define REFLECT_MEMBER(name) \
    ReflectMember(visitor, #name, value.name)
#define REFLECT_MEMBER2(name, value) \
    ReflectMember(visitor, name, value)


#define MAKE_REFLECT_TYPE_PROXY(type, as_type) \
  template<typename TVisitor> \
  void Reflect(TVisitor& visitor, type& value) { \
    auto value0 = static_cast<as_type>(value); \
    Reflect(visitor, value0); \
    value = static_cast<type>(value0); \
  }

#define _MAPPABLE_REFLECT_MEMBER(name) \
  REFLECT_MEMBER(name);

#define MAKE_REFLECT_EMPTY_STRUCT(type, ...) \
  template<typename TVisitor> \
  void Reflect(TVisitor& visitor, type& value) { \
    REFLECT_MEMBER_START(); \
    REFLECT_MEMBER_END(); \
  }

#define MAKE_REFLECT_STRUCT(type, ...) \
  template<typename TVisitor> \
  void Reflect(TVisitor& visitor, type& value) { \
    REFLECT_MEMBER_START(); \
    MACRO_MAP(_MAPPABLE_REFLECT_MEMBER, __VA_ARGS__) \
    REFLECT_MEMBER_END(); \
  }









template<typename T>
struct NonElidedVector : public std::vector<T> {};










// API:
/*
template<typename TVisitor, typename T>
void Reflect(TVisitor& visitor, T& value) {
  static_assert(false, "Missing implementation");
}
template<typename TVisitor>
void DefaultReflectMemberStart(TVisitor& visitor) {
  static_assert(false, "Missing implementation");
}
template<typename TVisitor, typename T>
bool ReflectMemberStart(TVisitor& visitor, T& value) {
  static_assert(false, "Missing implementation");
  return true;
}
template<typename TVisitor, typename T>
void ReflectMemberEnd(TVisitor& visitor, T& value) {
  static_assert(false, "Missing implementation");
}
*/



// int
void Reflect(Reader& visitor, int& value);
void Reflect(Writer& visitor, int& value);
// int64_t
void Reflect(Reader& visitor, int64_t& value);
void Reflect(Writer& visitor, int64_t& value);
// bool
void Reflect(Reader& visitor, bool& value);
void Reflect(Writer& visitor, bool& value);
// std::string
void Reflect(Reader& visitor, std::string& value);
void Reflect(Writer& visitor, std::string& value);







// Writer:
template<typename T>
void Reflect(Writer& visitor, std::vector<T>& values) {
  visitor.StartArray();
  for (auto& value : values)
    Reflect(visitor, value);
  visitor.EndArray();
}
template<typename T>
void Reflect(Writer& visitor, optional<T> value) {
  if (value)
    Reflect(visitor, value.value());
}
inline void DefaultReflectMemberStart(Writer& visitor) {
  visitor.StartObject();
}
template<typename T>
bool ReflectMemberStart(Writer& visitor, T& value) {
  visitor.StartObject();
  return true;
}
template<typename T>
void ReflectMemberEnd(Writer& visitor, T& value) {
  visitor.EndObject();
}
template<typename T>
void ReflectMember(Writer& visitor, const char* name, T& value) {
  visitor.Key(name);
  Reflect(visitor, value);
}
template<typename T>
void ReflectMember(Writer& visitor, const char* name, std::vector<T>& values) {
  if (values.empty())
    return;
  visitor.Key(name);
  visitor.StartArray();
  for (auto& value : values)
    Reflect(visitor, value);
  visitor.EndArray();
}
template<typename T>
void ReflectMember(Writer& visitor, const char* name, NonElidedVector<T>& values) {
  visitor.Key(name);
  visitor.StartArray();
  for (auto& value : values)
    Reflect(visitor, value);
  visitor.EndArray();
}
template<typename T>
void ReflectMember(Writer& visitor, const char* name, optional<T>& value) {
  if (!value)
    return;
  visitor.Key(name);
  Reflect(visitor, value);
}
void ReflectMember(Writer& visitor, const char* name, std::string& value);

// Reader:
template<typename T>
void Reflect(Reader& visitor, std::vector<T>& values) {
  for (auto& entry : visitor.GetArray()) {
    T entry_value;
    Reflect(entry, entry_value);
    values.push_back(entry_value);
  }
}
template<typename T>
void Reflect(Reader& visitor, optional<T>& value) {
  T real_value;
  Reflect(visitor, real_value);
  value = real_value;
}
inline void DefaultReflectMemberStart(Reader& visitor) {}
template<typename T>
bool ReflectMemberStart(Reader& visitor, T& value) {
  return true;
}
template<typename T>
void ReflectMemberEnd(Reader& visitor, T& value) {}
template<typename T>
void ReflectMember(Reader& visitor, const char* name, T& value) {
  auto it = visitor.FindMember(name);
  if (it != visitor.MemberEnd()) {
    Reader& child_visitor = it->value;
    Reflect(child_visitor, value);
  }
}

std::string Serialize(IndexedFile& file);
optional<IndexedFile> Deserialize(std::string path, std::string serialized);

void SetTestOutputMode();