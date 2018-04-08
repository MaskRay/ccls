#pragma once

#include "serializer.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

class JsonReader : public Reader {
  rapidjson::GenericValue<rapidjson::UTF8<>>* m_;
  std::vector<const char*> path_;

 public:
  JsonReader(rapidjson::GenericValue<rapidjson::UTF8<>>* m) : m_(m) {}
  SerializeFormat Format() const override { return SerializeFormat::Json; }

  bool IsBool() override { return m_->IsBool(); }
  bool IsNull() override { return m_->IsNull(); }
  bool IsInt() override { return m_->IsInt(); }
  bool IsInt64() override { return m_->IsInt64(); }
  bool IsUint64() override { return m_->IsUint64(); }
  bool IsDouble() override { return m_->IsDouble(); }
  bool IsString() override { return m_->IsString(); }

  void GetNull() override {}
  bool GetBool() override { return m_->GetBool(); }
  int GetInt() override { return m_->GetInt(); }
  uint32_t GetUint32() override { return uint32_t(m_->GetUint64()); }
  int64_t GetInt64() override { return m_->GetInt64(); }
  uint64_t GetUint64() override { return m_->GetUint64(); }
  double GetDouble() override { return m_->GetDouble(); }
  std::string GetString() override { return m_->GetString(); }

  bool HasMember(const char* x) override { return m_->HasMember(x); }
  std::unique_ptr<Reader> operator[](const char* x) override {
    auto& sub = (*m_)[x];
    return std::unique_ptr<JsonReader>(new JsonReader(&sub));
  }

  void IterArray(std::function<void(Reader&)> fn) override {
    if (!m_->IsArray())
      throw std::invalid_argument("array");
    // Use "0" to indicate any element for now.
    path_.push_back("0");
    for (auto& entry : m_->GetArray()) {
      auto saved = m_;
      m_ = &entry;
      fn(*this);
      m_ = saved;
    }
    path_.pop_back();
  }

  void DoMember(const char* name, std::function<void(Reader&)> fn) override {
    path_.push_back(name);
    auto it = m_->FindMember(name);
    if (it != m_->MemberEnd()) {
      auto saved = m_;
      m_ = &it->value;
      fn(*this);
      m_ = saved;
    }
    path_.pop_back();
  }

  std::string GetPath() const {
    std::string ret;
    for (auto& t : path_) {
      ret += '/';
      ret += t;
    }
    ret.pop_back();
    return ret;
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
  void Uint32(uint32_t x) override { m_->Uint64(x); }
  void Int64(int64_t x) override { m_->Int64(x); }
  void Uint64(uint64_t x) override { m_->Uint64(x); }
  void Double(double x) override { m_->Double(x); }
  void String(const char* x) override { m_->String(x); }
  void String(const char* x, size_t len) override { m_->String(x, len); }
  void StartArray(size_t) override { m_->StartArray(); }
  void EndArray() override { m_->EndArray(); }
  void StartObject() override { m_->StartObject(); }
  void EndObject() override { m_->EndObject(); }
  void Key(const char* name) override { m_->Key(name); }
};
