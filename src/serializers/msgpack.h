#pragma once

#include "serializer.h"

#include <msgpack.hpp>

class MessagePackReader : public Reader {
  msgpack::object o_;
  size_t idx_ = 0;
  std::vector<msgpack::object> children_;

 public:
  MessagePackReader(msgpack::object o) : o_(o) {}
  SerializeFormat Format() const override { return SerializeFormat::MessagePack; }

  bool IsNull() override { return o_.is_nil(); }
  bool IsArray() override { return o_.type == msgpack::type::ARRAY; }
  bool IsInt() override {
    return o_.type == msgpack::type::POSITIVE_INTEGER ||
           o_.type == msgpack::type::NEGATIVE_INTEGER;
  }
  bool IsString() override { return o_.type == msgpack::type::STR; }

  bool GetBool() override { return o_.as<bool>(); }
  int GetInt() override { return o_.as<int>(); }
  int64_t GetInt64() override { return o_.as<int64_t>(); }
  uint64_t GetUint64() override { return o_.as<uint64_t>(); }
  std::string GetString() override { return o_.as<std::string>(); }

  bool HasMember(const char* x) override { return true; }
  std::unique_ptr<Reader> operator[](const char* x) override {
    return {};
  }

  void IterArray(std::function<void(Reader&)> fn) override {
    for (auto& entry : o_.as<std::vector<msgpack::object>>()) {
      MessagePackReader sub(entry);
      fn(sub);
    }
  }

  void DoMember(const char* name, std::function<void(Reader&)> fn) override {
    if (idx_ == 0)
      children_ = o_.as<std::vector<msgpack::object>>();
    MessagePackReader sub(children_[idx_++]);
    fn(sub);
  }
};

class MessagePackWriter : public Writer {
  msgpack::packer<msgpack::sbuffer>* m_;

 public:
  MessagePackWriter(msgpack::packer<msgpack::sbuffer>* m) : m_(m) {}
  SerializeFormat Format() const override { return SerializeFormat::MessagePack; }

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
  void StartObject(size_t n) override { m_->pack_array(uint32_t(n)); }
  void EndObject() override {}
  void Key(const char* name) override {}
};
