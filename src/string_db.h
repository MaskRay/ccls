#if false
#pragma once

#include "buffer.h"
#include "utils.h"

#include <clang-c/Index.h>

#include <cassert>
#include <cstring>
#include <cstdint>
#include <functional>
#include <unordered_set>
#include <string>


template <typename T>
struct StringView {
  StringView();
  StringView(const StringView& that);
  StringView(const char* str, size_t len);

  bool operator==(const StringView& that) const;
  bool operator!=(const StringView& that) const;
  bool operator<(const StringView& that) const;
  StringView& operator=(const StringView& that);

  std::string AsString() const;

  size_t len;
  const char* str;
};
// See MAKE_HASHABLE for QueryUsr, IndexUsr

struct StringStorage {
  ~StringStorage();

  static StringStorage CreateUnowned(const char* str, size_t len);
  void Copy() const;

  bool operator==(const StringStorage& that) const;
  bool operator!=(const StringStorage& that) const;
  bool operator<(const StringStorage& that) const;

  size_t len;
  mutable const char* str;
  mutable bool owns_str;
};
MAKE_HASHABLE(StringStorage, t.len, t.str);

template <typename T>
struct StringDb {
  TStringView<T> GetString(const char* str, size_t len);
  TStringView<T> GetString(const std::string& str);
  TStringView<T> GetString(CXString cx_string);

  std::unordered_set<StringStorage> data_;
};








struct _DummyQueryType {};
struct _DummyIndexType {};

using QueryUsr = StringView<_DummyQueryType>;
using QueryStringDb = StringDb<_DummyQueryType>;

using IndexUsr = StringView<_DummyIndexType>;
using IndexStringDb = StringDb<_DummyIndexType>;



// TODO: See if we can move this next to StringView definition.
MAKE_HASHABLE(QueryUsr, t.len, t.str);
MAKE_HASHABLE(IndexUsr, t.len, t.str);













template <typename T>
StringView<T>::StringView() : len(0), str(nullptr) {}

template <typename T>
StringView<T>::StringView(const StringView& that) : len(that.len), str(that.str) {}

template <typename T>
StringView<T>::StringView(const char* str, size_t len) : len(len), str(str) {}

template <typename T>
bool StringView<T>::operator==(const StringView& that) const {
  return len == that.len && strcmp(str, that.str) == 0;
}

template <typename T>
bool StringView<T>::operator!=(const StringView& that) const {
  return !(*this == that);
}

template <typename T>
bool StringView<T>::operator<(const StringView& that) const {
  return strcmp(str, that.str);
}

template <typename T>
StringView& StringView<T>::operator=(const StringView& that) {
  len = that.len;
  str = that.str;
  return *this;
}

template <typename T>
std::string StringView<T>::AsString() const {
  return std::string(str, len);
}

template <typename T>
TStringView<T> StringDb<T>::GetString(const char* str, size_t len) {
  StringStorage lookup = StringStorage::CreateUnowned(str, len);

  auto it = data_.insert(lookup);
  if (it.second)
    it.first->Copy();

  return TStringView<T>(it.first->str, it.first->len);
}

template <typename T>
TStringView<T> StringDb<T>::GetString(const std::string& str) {
  return GetString(str.c_str());
}

template <typename T>
TStringView<T> StringDb<T>::GetString(CXString cx_string) {
  assert(cx_string.data);
  const char* str = clang_getCString(cx_string);
  StringView result = GetString(str);
  clang_disposeString(cx_string);
  return result;
}
#endif