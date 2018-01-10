#pragma once

#include <macro_map.h>
#include <optional.h>
#include <variant.h>

#include <memory>
#include <string>
#include <vector>

enum class SerializeFormat { Json, MessagePack };

class Reader {
 public:
  virtual ~Reader() {}
  virtual SerializeFormat Format() const = 0;

  //virtual bool IsBool() = 0;
  virtual bool IsNull() = 0;
  virtual bool IsArray() = 0;
  virtual bool IsInt() = 0;
  //virtual bool IsInt64() = 0;
  //virtual bool IsUint64() = 0;
  virtual bool IsString() = 0;

  virtual void GetNull() = 0;
  virtual bool GetBool() = 0;
  virtual int GetInt() = 0;
  virtual int64_t GetInt64() = 0;
  virtual uint64_t GetUint64() = 0;
  virtual double GetDouble() = 0;
  virtual std::string GetString() = 0;

  virtual bool HasMember(const char* x) = 0;
  virtual std::unique_ptr<Reader> operator[](const char* x) = 0;

  virtual void IterArray(std::function<void(Reader&)> fn) = 0;
  virtual void DoMember(const char* name, std::function<void(Reader&)> fn) = 0;
};

class Writer {
 public:
  virtual ~Writer() {}
  virtual SerializeFormat Format() const = 0;

  virtual void Null() = 0;
  virtual void Bool(bool x) = 0;
  virtual void Int(int x) = 0;
  virtual void Int64(int64_t x) = 0;
  virtual void Uint64(uint64_t x) = 0;
  virtual void Double(double x) = 0;
  virtual void String(const char* x) = 0;
  virtual void String(const char* x, size_t len) = 0;
  virtual void StartArray(size_t) = 0;
  virtual void EndArray() = 0;
  virtual void StartObject() = 0;
  virtual void EndObject() = 0;
  virtual void Key(const char* name) = 0;
};

struct IndexFile;

#define REFLECT_MEMBER_START()                  \
  if (!ReflectMemberStart(visitor, value))      \
    return
#define REFLECT_MEMBER_START1(value)            \
  if (!ReflectMemberStart(visitor, value))      \
    return
#define REFLECT_MEMBER_END() ReflectMemberEnd(visitor, value);
#define REFLECT_MEMBER_END1(value) ReflectMemberEnd(visitor, value);
#define REFLECT_MEMBER(name) ReflectMember(visitor, #name, value.name)
#define REFLECT_MEMBER2(name, value) ReflectMember(visitor, name, value)

#define MAKE_REFLECT_TYPE_PROXY(type, as_type)   \
  template <typename TVisitor>                   \
  void Reflect(TVisitor& visitor, type& value) { \
    auto value0 = static_cast<as_type>(value);   \
    ::Reflect(visitor, value0);                  \
    value = static_cast<type>(value0);           \
  }

#define _MAPPABLE_REFLECT_MEMBER(name) REFLECT_MEMBER(name);

#define MAKE_REFLECT_EMPTY_STRUCT(type, ...)     \
  template <typename TVisitor>                   \
  void Reflect(TVisitor& visitor, type& value) { \
    REFLECT_MEMBER_START();                      \
    REFLECT_MEMBER_END();                        \
  }

#define MAKE_REFLECT_STRUCT(type, ...)               \
  template <typename TVisitor>                       \
  void Reflect(TVisitor& visitor, type& value) {     \
    REFLECT_MEMBER_START();                          \
    MACRO_MAP(_MAPPABLE_REFLECT_MEMBER, __VA_ARGS__) \
    REFLECT_MEMBER_END();                            \
  }

// clang-format off
// Config has many fields, we need to support at least its number of fields.
#define NUM_VA_ARGS_IMPL(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,N,...) N
#define NUM_VA_ARGS(...) NUM_VA_ARGS_IMPL(__VA_ARGS__,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1)
// clang-format on

#define _MAPPABLE_REFLECT_ARRAY(name) Reflect(visitor, value.name);

// Reflects the struct so it is serialized as an array instead of an object.
// This currently only supports writers.
#define MAKE_REFLECT_STRUCT_WRITER_AS_ARRAY(type, ...)  \
  inline void Reflect(Writer& visitor, type& value) {   \
    visitor.StartArray(NUM_VA_ARGS(__VA_ARGS__));       \
    MACRO_MAP(_MAPPABLE_REFLECT_ARRAY, __VA_ARGS__)     \
        visitor.EndArray();                             \
  }

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

// uint8_t
void Reflect(Reader& visitor, uint8_t& value);
void Reflect(Writer& visitor, uint8_t& value);
// int16_t
void Reflect(Reader& visitor, int16_t& value);
void Reflect(Writer& visitor, int16_t& value);
// int32_t
void Reflect(Reader& visitor, int32_t& value);
void Reflect(Writer& visitor, int32_t& value);
// int64_t
void Reflect(Reader& visitor, int64_t& value);
void Reflect(Writer& visitor, int64_t& value);
// uint64_t
void Reflect(Reader& visitor, uint64_t& value);
void Reflect(Writer& visitor, uint64_t& value);
// double
void Reflect(Reader& visitor, double& value);
void Reflect(Writer& visitor, double& value);
// bool
void Reflect(Reader& visitor, bool& value);
void Reflect(Writer& visitor, bool& value);

// std::string
void Reflect(Reader& visitor, std::string& value);
void Reflect(Writer& visitor, std::string& value);

void Reflect(Reader& visitor, std::monostate&);
void Reflect(Writer& visitor, std::monostate&);

void Reflect(Reader& visitor, SerializeFormat& value);
void Reflect(Writer& visitor, SerializeFormat& value);

// std::optional
template <typename T>
void Reflect(Reader& visitor, optional<T>& value) {
  if (visitor.IsNull()) {
    visitor.GetNull();
    return;
  }
  T real_value{};
  Reflect(visitor, real_value);
  value = real_value;
}
template <typename T>
void Reflect(Writer& visitor, optional<T>& value) {
  // We omit optional fields for JsonWriter to reduce output.
  // But don't omit them for other serialization formats.
  if (value)
    Reflect(visitor, *value);
  else if (visitor.Format() != SerializeFormat::Json)
    visitor.Null();
}

template <size_t N, typename... Ts>
struct ReflectVariant {
  void operator()(Writer& visitor, std::variant<Ts...>& value) {
    if (value.index() == N - 1)
      Reflect(visitor, std::get<N - 1>(value));
    else
      ReflectVariant<N - 1, Ts...>()(visitor, value);
  }
};

template <typename... Ts>
struct ReflectVariant<1, Ts...> {
  void operator()(Writer& visitor, std::variant<Ts...>& value) {
    Reflect(visitor, std::get<0>(value));
  }
};

// std::variant (Writer only)
template <typename... Ts>
void Reflect(Writer& visitor, std::variant<Ts...>& value) {
  ReflectVariant<sizeof...(Ts), Ts...>()(visitor, value);
}

// std::vector
template <typename T>
void Reflect(Reader& visitor, std::vector<T>& values) {
  visitor.IterArray([&](Reader& entry) {
    T entry_value;
    Reflect(entry, entry_value);
    values.push_back(entry_value);
  });
}
template <typename T>
void Reflect(Writer& visitor, std::vector<T>& values) {
  visitor.StartArray(values.size());
  for (auto& value : values)
    Reflect(visitor, value);
  visitor.EndArray();
}

// Writer:

inline void DefaultReflectMemberStart(Writer& visitor) {
  visitor.StartObject();
}
template <typename T>
bool ReflectMemberStart(Writer& visitor, T& value) {
  visitor.StartObject();
  return true;
}
template <typename T>
void ReflectMemberEnd(Writer& visitor, T& value) {
  visitor.EndObject();
}
template <typename T>
void ReflectMember(Writer& visitor, const char* name, T& value) {
  visitor.Key(name);
  Reflect(visitor, value);
}
template <typename T>
void ReflectMember(Writer& visitor, const char* name, std::vector<T>& values) {
  visitor.Key(name);
  visitor.StartArray(values.size());
  for (auto& value : values)
    Reflect(visitor, value);
  visitor.EndArray();
}
template <typename T>
void ReflectMember(Writer& visitor, const char* name, optional<T>& value) {
  if (value || visitor.Format() != SerializeFormat::Json) {
    visitor.Key(name);
    Reflect(visitor, value);
  }
}
void ReflectMember(Writer& visitor, const char* name, std::string& value);

// Reader:


inline void DefaultReflectMemberStart(Reader& visitor) {}
template <typename T>
bool ReflectMemberStart(Reader& visitor, T& value) {
  return true;
}
template <typename T>
void ReflectMemberEnd(Reader& visitor, T& value) {}
template <typename T>
void ReflectMember(Reader& visitor, const char* name, T& value) {
  visitor.DoMember(name, [&](Reader& child) { Reflect(child, value); });
}

// API

std::string Serialize(SerializeFormat format, IndexFile& file);
std::unique_ptr<IndexFile> Deserialize(SerializeFormat format,
                                       std::string path,
                                       std::string serialized,
                                       optional<int> expected_version);

void SetTestOutputMode();
