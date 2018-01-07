#pragma once

#include "serializer.h"

#include <msgpack.hpp>

class MessagePackReader : public Reader {
  msgpack::unpacker* m_;
  msgpack::object_handle oh_;

  void next() { m_->next(oh_); }

 public:
  MessagePackReader(msgpack::unpacker* m) : m_(m) { next(); }
  SerializeFormat Format() const override { return SerializeFormat::MessagePack; }

  bool IsNull() override { return oh_.get().is_nil(); }
  bool IsArray() override { return oh_.get().type == msgpack::type::ARRAY; }
  bool IsInt() override {
    return oh_.get().type == msgpack::type::POSITIVE_INTEGER ||
           oh_.get().type == msgpack::type::NEGATIVE_INTEGER;
  }
  bool IsString() override { return oh_.get().type == msgpack::type::STR; }

  bool GetBool() override {
    auto ret = oh_.get().as<bool>();
    next();
    return ret;
  }
  int GetInt() override {
    auto ret = oh_.get().as<int>();
    next();
    return ret;
  }
  int64_t GetInt64() override {
    auto ret = oh_.get().as<int64_t>();
    next();
    return ret;
  }
  uint64_t GetUint64() override {
    auto ret = oh_.get().as<uint64_t>();
    next();
    return ret;
  }
  const char* GetCString() override {
    auto ret = oh_.get().as<char*>();
    next();
    return ret;
  }

  bool HasMember(const char* x) override { return true; }
  std::unique_ptr<Reader> operator[](const char* x) override {
    return {};
  }

  void IterArray(std::function<void(Reader&)> fn) override {
  }

  void DoMember(const char* name, std::function<void(Reader&)> fn) override {
    const char* key = GetCString();
    fn(*this);
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
  // TODO pack_array
  void StartObject(size_t n) override { m_->pack_map(uint32_t(n)); }
  void EndObject() override {}
  void Key(const char* name) override { m_->pack(name); }
};
