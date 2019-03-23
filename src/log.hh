#pragma once

#include <sstream>
#include <stdio.h>
#include <string>
#include <type_traits>

namespace ccls::log {
extern FILE *file;

enum class Verbosity : signed {
  FATAL = -3,
  ERROR = -2,
  WARNING = -1,
  INFO = 0,
  DEBUG = 1,
  VERBOSE = 2
};
template <typename T, typename UT = std::underlying_type_t<T>>
constexpr auto operator+(T e) noexcept
    -> std::enable_if_t<std::is_enum_v<T>, UT> {
  return static_cast<UT>(e);
}

extern Verbosity verbosity;

struct Message {
  std::stringstream stream_;
  Verbosity verbosity_;

  Message(Verbosity verbosity, const char *file, int line);
  ~Message();
};

template <typename... Args> inline void Log(Verbosity v, Args const &... args) {
  (Message(v, __FILE__, __LINE__).stream_ << ... << args);
}

// XXX: According to CWG 1766, static_cast invalid(out of range) value to enum
// class is UB
bool inline LogRequire(Verbosity v) { return +v <= +verbosity; }

// ADL
bool inline LogIf(Verbosity v, bool cond) { return cond && LogRequire(v); }
} // namespace ccls::log
