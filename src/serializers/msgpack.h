#pragma once

#include "serializer.h"

#include <msgpack.hpp>

class MessagePackReader : public Reader {
  msgpack::unpacker* pk_;
  msgpack::object_handle oh_;

  template <typename T>
  T Get() {
    T ret = oh_.get().as<T>();
    pk_->next(oh_);
    return ret;
  }

 public:
  MessagePackReader(msgpack::unpacker* pk) : pk_(pk) { pk->next(oh_); }
  SerializeFormat Format() const override {
    return SerializeFormat::MessagePack;
  }

  bool IsNull() override { return oh_.get().is_nil(); }
  bool IsArray() override { return oh_.get().type == msgpack::type::ARRAY; }
  bool IsInt() override {
    return oh_.get().type == msgpack::type::POSITIVE_INTEGER ||
           oh_.get().type == msgpack::type::NEGATIVE_INTEGER;
  }
  bool IsString() override { return oh_.get().type == msgpack::type::STR; }

  void GetNull() override { pk_->next(oh_); }
  bool GetBool() override { return Get<bool>(); }
  int GetInt() override { return Get<int>(); }
  int64_t GetInt64() override { return Get<int64_t>(); }
  uint64_t GetUint64() override { return Get<uint64_t>(); }
  double GetDouble() override { return Get<double>(); }
  std::string GetString() override { return Get<std::string>(); }

  bool HasMember(const char* x) override { return true; }
  std::unique_ptr<Reader> operator[](const char* x) override { return {}; }

  void IterArray(std::function<void(Reader&)> fn) override {
    size_t n = Get<size_t>();
    for (size_t i = 0; i < n; i++)
      fn(*this);
  }

  void DoMember(const char*, std::function<void(Reader&)> fn) override {
    fn(*this);
  }
};

class MessagePackWriter : public Writer {
  msgpack::packer<msgpack::sbuffer>* m_;

 public:
  MessagePackWriter(msgpack::packer<msgpack::sbuffer>* m) : m_(m) {}
  SerializeFormat Format() const override {
    return SerializeFormat::MessagePack;
  }

  void Null() override { m_->pack_nil(); }
  void Bool(bool x) override { m_->pack(x); }
  void Int(int x) override { m_->pack(x); }
  void Int64(int64_t x) override { m_->pack(x); }
  void Uint64(uint64_t x) override { m_->pack(x); }
  void Double(double x) override { m_->pack(x); }
  void String(const char* x) override { m_->pack(x); }
  // TODO Remove std::string
  void String(const char* x, size_t len) override {
    m_->pack(std::string(x, len));
  }
  void StartArray(size_t n) override { m_->pack(n); }
  void EndArray() override {}
  void StartObject() override {}
  void EndObject() override {}
  void Key(const char* name) override {}
};
