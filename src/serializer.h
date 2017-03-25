#pragma once

#include <optional.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include <vector>
#include <string>


using std::experimental::optional;
using std::experimental::nullopt;

using Reader = rapidjson::GenericValue<rapidjson::UTF8<>>;
using Writer = rapidjson::PrettyWriter<rapidjson::StringBuffer>;
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














#if false

void Serialize(Writer& writer, int value);
void Serialize(Writer& writer, const std::string& value);
void Serialize(Writer& writer, Location location);
void Serialize(Writer& writer, uint64_t value);
void Serialize(Writer& writer, IndexedFile& file);

template<typename T>
void Serialize(Writer& writer, Id<T> id) {
  writer.Uint64(id.id);
}

template<typename T>
void Serialize(Writer& writer, optional<T> value) {
  if (value)
    Serialize(writer, value.value());
  else
    writer.Null();
}

template<typename T>
void Serialize(Writer& writer, const std::vector<T>& values) {
  writer.StartArray();
  for (const T& value : values)
    Serialize(writer, value);
  writer.EndArray();
}

template<typename T>
void Serialize(Writer& writer, Ref<T> ref) {
  std::string s = std::to_string(ref.id.id) + "@" + ref.loc.ToString();
  writer.String(s.c_str());
}

template<typename T>
void Serialize(Writer& writer, const char* key, const std::vector<Ref<T>>& refs) {
  if (refs.size() == 0)
    return;

  if (key) writer.Key(key);
  writer.StartArray();
  for (Ref<T> ref : refs)
    Serialize(writer, nullptr, ref);
  writer.EndArray();
}





#define SERIALIZE_MEMBER(name) \
    SerializeMember(writer, #name, value.name)
#define SERIALIZE_MEMBER2(name, value) \
    SerializeMember(writer, name, value)
#define DESERIALIZE_MEMBER(name) \
    DeserializeMember(reader, #name, value.name)
#define DESERIALIZE_MEMBER2(name, value) \
    DeserializeMember(reader, name, value)

// Special templates used by (DE)SERIALIZE_MEMBER macros.
template<typename T>
void SerializeMember(Writer& writer, const char* name, const T& value) {
  writer.Key(name);
  Serialize(writer, value);
}
template<typename T>
void SerializeMember(Writer& writer, const char* name, const std::vector<T>& value) {
  if (value.empty())
    return;
  writer.Key(name);
  Serialize(writer, value);
}
template<typename T>
void SerializeMember(Writer& writer, const char* name, const optional<T>& value) {
  if (!value)
    return;
  writer.Key(name);
  Serialize(writer, value.value());
}

void SerializeMember(Writer& writer, const char* name, const std::string& value);

template<typename T>
void DeserializeMember(const Reader& reader, const char* name, T& value) {
  auto it = reader.FindMember(name);
  if (it != reader.MemberEnd())
    Deserialize(it->value, value);
}







template<typename T>
void Deserialize(const Reader& reader, Id<T>& output) {
  output = Id<T>(reader.GetUint64());
}

template<typename T>
void Deserialize(const Reader& reader, Ref<T>& output) {
  const char* str_value = reader.GetString();
  uint64_t id = atoi(str_value);
  const char* loc_string = strchr(str_value, '@') + 1;

  output.id = Id<T>(id);
  output.loc = Location(loc_string);
}

template<typename T>
void Deserialize(const Reader& reader, std::vector<T>& value) {
  for (const auto& entry : reader.GetArray()) {
    T entry_value;
    Deserialize(entry, entry_value);
    value.push_back(entry_value);
  }
}

template<typename T>
void Deserialize(const Reader& reader, optional<T>& value) {
  T real_value;
  Deserialize(reader, real_value);
  value = real_value;
}

void Deserialize(const Reader& reader, int& value);
void Deserialize(const Reader& reader, bool& value);
void Deserialize(const Reader& reader, std::string& value);
void Deserialize(const Reader& reader, Location& output);
void Deserialize(const Reader& reader, IndexedTypeDef& value);
void Deserialize(const Reader& reader, IndexedFuncDef& value);
void Deserialize(const Reader& reader, IndexedVarDef& value);
void Deserialize(const Reader& reader, IndexedFile& file);

#endif

std::string Serialize(IndexedFile& file);
IndexedFile Deserialize(std::string path, std::string serialized);
