// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "serializer.h"

#include <string.h>

class BinaryReader : public Reader {
  const char *p_;

  template <typename T> T Get() {
    T ret;
    memcpy(&ret, p_, sizeof(T));
    p_ += sizeof(T);
    return ret;
  }

  uint64_t VarUInt() {
    auto x = *reinterpret_cast<const uint8_t *>(p_++);
    if (x < 253)
      return x;
    if (x == 253)
      return Get<uint16_t>();
    if (x == 254)
      return Get<uint32_t>();
    return Get<uint64_t>();
  }
  int64_t VarInt() {
    uint64_t x = VarUInt();
    return int64_t(x >> 1 ^ -(x & 1));
  }

public:
  BinaryReader(std::string_view buf) : p_(buf.data()) {}
  SerializeFormat Format() const override { return SerializeFormat::Binary; }

  bool IsBool() override { return true; }
  // Abuse how the function is called in serializer.h
  bool IsNull() override { return !*p_++; }
  bool IsInt() override { return true; }
  bool IsInt64() override { return true; }
  bool IsUInt64() override { return true; }
  bool IsDouble() override { return true; }
  bool IsString() override { return true; }

  void GetNull() override {}
  bool GetBool() override { return Get<bool>(); }
  int GetInt() override { return VarInt(); }
  int64_t GetInt64() override { return VarInt(); }
  uint8_t GetUInt8() override { return Get<uint8_t>(); }
  uint32_t GetUInt32() override { return VarUInt(); }
  uint64_t GetUInt64() override { return VarUInt(); }
  double GetDouble() override { return Get<double>(); }
  const char *GetString() override {
    const char *ret = p_;
    while (*p_)
      p_++;
    p_++;
    return ret;
  }

  bool HasMember(const char *x) override { return true; }
  std::unique_ptr<Reader> operator[](const char *x) override { return {}; }

  void IterArray(std::function<void(Reader &)> fn) override {
    for (auto n = VarUInt(); n; n--)
      fn(*this);
  }

  void Member(const char *, std::function<void()> fn) override { fn(); }
};

class BinaryWriter : public Writer {
  std::string buf_;

  template <typename T> void Pack(T x) {
    auto i = buf_.size();
    buf_.resize(i + sizeof(x));
    memcpy(buf_.data() + i, &x, sizeof(x));
  }

  void VarUInt(uint64_t n) {
    if (n < 253)
      Pack<uint8_t>(n);
    else if (n < 65536) {
      Pack<uint8_t>(253);
      Pack<uint16_t>(n);
    } else if (n < 4294967296) {
      Pack<uint8_t>(254);
      Pack<uint32_t>(n);
    } else {
      Pack<uint8_t>(255);
      Pack<uint64_t>(n);
    }
  }
  void VarInt(int64_t n) { VarUInt(uint64_t(n) << 1 ^ n >> 63); }

public:
  SerializeFormat Format() const override { return SerializeFormat::Binary; }
  std::string Take() { return std::move(buf_); }

  void Null() override { Pack(uint8_t(0)); }
  void Bool(bool x) override { Pack(x); }
  void Int(int x) override { VarInt(x); }
  void Int64(int64_t x) override { VarInt(x); }
  void UInt8(uint8_t x) override { Pack(x); }
  void UInt32(uint32_t x) override { VarUInt(x); }
  void UInt64(uint64_t x) override { VarUInt(x); }
  void Double(double x) override { Pack(x); }
  void String(const char *x) override { String(x, strlen(x)); }
  void String(const char *x, size_t len) override {
    auto i = buf_.size();
    buf_.resize(i + len + 1);
    memcpy(buf_.data() + i, x, len);
  }
  void StartArray(size_t n) override { VarUInt(n); }
  void EndArray() override {}
  void StartObject() override {}
  void EndObject() override {}
  void Key(const char *name) override {}
};
