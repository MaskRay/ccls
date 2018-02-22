#pragma once

#include <string_view.h>

#include <string.h>
#include <memory>
#include <string>
#include <utility>

// Nullable null-terminated string, which is null if default constructed,
// but non-null if assigned.
// This is used in Query{Func,Type,Var}::def to reduce memory footprint.
class NtString {
  using size_type = std::string::size_type;
  std::unique_ptr<char[]> str;

 public:
  NtString() = default;
  NtString(NtString&& o) = default;
  NtString(const NtString& o) { *this = o; }
  NtString(std::string_view sv) { *this = sv; }

  const char* c_str() const { return str.get(); }
  operator std::string_view() const {
    if (c_str())
      return c_str();
    return {};
  }
  bool empty() const { return !str || *c_str() == '\0'; }

  void operator=(std::string_view sv) {
    str = std::unique_ptr<char[]>(new char[sv.size() + 1]);
    memcpy(str.get(), sv.data(), sv.size());
    str.get()[sv.size()] = '\0';
  }
  void operator=(const NtString& o) {
    *this = static_cast<std::string_view>(o);
  }
  bool operator==(const NtString& o) const {
    return str && o.str ? strcmp(c_str(), o.c_str()) == 0
                        : c_str() == o.c_str();
  }
};
