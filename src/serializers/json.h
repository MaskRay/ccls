#pragma once

#include "serializer.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

class JsonReader : public Reader {
  rapidjson::GenericValue<rapidjson::UTF8<>>* m_;

 public:
  JsonReader(rapidjson::GenericValue<rapidjson::UTF8<>>* m) : m_(m) {}
  SerializeFormat Format() const override { return SerializeFormat::Json; }

  //bool IsBool() override { return m_->IsBool(); }
  bool IsNull() override { return m_->IsNull(); }
  bool IsArray() override { return m_->IsArray(); }
  bool IsInt() override { return m_->IsInt(); }
  //bool IsInt64() override { return m_->IsInt64(); }
  //bool IsUint64() override { return m_->IsUint64(); }
  bool IsString() override { return m_->IsString(); }

  bool GetBool() override { return m_->GetBool(); }
  int GetInt() override { return m_->GetInt(); }
  int64_t GetInt64() override { return m_->GetInt64(); }
  uint64_t GetUint64() override { return m_->GetUint64(); }
  std::string GetString() override { return m_->GetString(); }

  bool HasMember(const char* x) override { return m_->HasMember(x); }
  std::unique_ptr<Reader> operator[](const char* x) override {
    auto& sub = (*m_)[x];
    return std::unique_ptr<JsonReader>(new JsonReader(&sub));
  }

  void IterArray(std::function<void(Reader&)> fn) override {
    for (auto& entry : m_->GetArray()) {
      JsonReader sub(&entry);
      fn(sub);
    }
  }

  void DoMember(const char* name, std::function<void(Reader&)> fn) override {
    if (m_->GetType() != rapidjson::Type::kObjectType)
      return; // FIXME: signal an error that object was not deserialized correctly?

    auto it = m_->FindMember(name);
    if (it != m_->MemberEnd()) {
      JsonReader sub(&it->value);
      fn(sub);
    }
  }
};

class JsonWriter : public Writer {
  rapidjson::Writer<rapidjson::StringBuffer>* m_;

 public:
  JsonWriter(rapidjson::Writer<rapidjson::StringBuffer>* m) : m_(m) {}
  SerializeFormat Format() const override { return SerializeFormat::Json; }

  void Null() override { m_->Null(); }
  void Bool(bool x) override { m_->Bool(x); }
  void Int(int x) override { m_->Int(x); }
  void Int64(int64_t x) override { m_->Int64(x); }
  void Uint64(uint64_t x) override { m_->Uint64(x); }
  void String(const char* x) override { m_->String(x); }
  void String(const char* x, size_t len) override { m_->String(x, len); }
  void StartArray(size_t) override { m_->StartArray(); }
  void EndArray() override { m_->EndArray(); }
  void StartObject(size_t) override { m_->StartObject(); }
  void EndObject() override { m_->EndObject(); }
  void Key(const char* name) override { m_->Key(name); }
};
