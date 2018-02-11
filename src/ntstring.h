#pragma once

#include <string_view.h>

#include <string.h>
#include <memory>
#include <utility>

class Reader;
class Writer;

// Null-terminated string
// This is used in Query{Func,Type,Var}::def to reduce memory footprint.
class NTString {
  using size_type = std::string::size_type;
  std::unique_ptr<char[]> str;

 public:
  NTString() : str(new char[1]()) {}
  NTString(const NTString& o) : str(new char[strlen(o.c_str()) + 1]) {
    strcpy(str.get(), o.c_str());
  }
  NTString(NTString&& o) = default;
  NTString(std::string_view sv) {
    *this = sv;
  }

  operator std::string_view() const { return str.get(); }
  const char* c_str() const { return str.get(); }
  std::string_view substr(size_type pos, size_type cnt) const {
    return std::string_view(c_str() + pos, cnt);
  }
  bool empty() const { return !str || str.get()[0] == '\0'; }
  size_type find(const char* s) {
    const char *p = strstr(c_str(), s);
    return p ? std::string::size_type(p - c_str()) : std::string::npos;
  }

  void operator=(std::string_view sv) {
    str = std::unique_ptr<char[]>(new char[sv.size() + 1]);
    memcpy(str.get(), sv.data(), sv.size());
    str.get()[sv.size()] = '\0';
  }
  void operator=(const NTString& o) {
    *this = static_cast<std::string_view>(o);
  }
  bool operator==(const NTString& o) const {
    return strcmp(c_str(), o.c_str()) == 0;
  }
};
