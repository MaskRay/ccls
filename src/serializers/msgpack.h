#pragma once

#include "serializer.h"

#include <msgpack.hpp>

class MessagePackReader : public Reader {
  msgpack::object_handle* m_;

 public:
  MessagePackReader(msgpack::object_handle* m) : m_(m) {}

  //bool GetBool() override { return m_->GetBool(); }
  //int GetInt() override { return m_->GetInt(); }
  //int64_t GetInt64() override { return m_->GetInt64(); }
  //uint64_t GetUint64() override { return m_->GetUint64(); }
  //const char* GetString() override { return m_->GetString(); }

  //bool HasMember(const char* x) override { return m_->HasMember(x); }
  //std::unique_ptr<Reader> operator[](const char* x) override {
  //  auto& sub = (*m_)[x];
  //  return std::unique_ptr<JsonReader>(new JsonReader(&sub));
  //}

  //void IterArray(std::function<void(Reader&)> fn) override {
  //  for (auto& entry : m_->GetArray()) {
  //    MessagePackReader sub(&entry);
  //    fn(sub);
  //  }
  //}

  //void DoMember(const char* name, std::function<void(Reader&)> fn) override {
  //  auto it = m_->FindMember(name);
  //  if (it != m_->MemberEnd()) {
  //    MessagePackReader sub(&it->value);
  //    fn(sub);
  //  }
  //}
};

class MessagePackWriter : public Writer {
  msgpack::packer<msgpack::sbuffer>* m_;

 public:
  MessagePackWriter(msgpack::packer<msgpack::sbuffer>* m) : m_(m) {}

  void Null() override { m_->pack_nil(); }
  void Bool(bool x) override { m_->pack(x); }
  void Int(int x) override { m_->pack(x); }
  void Int64(int64_t x) override { m_->pack(x); }
  void Uint64(uint64_t x) override { m_->pack(x); }
  void String(const char* x) override { m_->pack(x); }
  // TODO Remove std::string
  void String(const char* x, size_t len) override { m_->pack(std::string(x, len)); }
  void StartArray(size_t n) override { m_->pack_array(uint32_t(n)); }
  void EndArray() override {}
  // TODO pack_array
  void StartObject(size_t n) override { m_->pack_map(uint32_t(n)); }
  void EndObject() override {}
  void Key(const char* name) override { m_->pack(name); }
};
