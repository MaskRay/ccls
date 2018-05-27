#pragma once

#include <stdio.h>
#include <sstream>

namespace ccls::log {
extern FILE* file;

struct Voidify {
  void operator&(const std::ostream&) {}
};

enum Verbosity {
  Verbosity_FATAL = -3,
  Verbosity_ERROR = -2,
  Verbosity_WARNING = -1,
  Verbosity_INFO = 0,
};
extern Verbosity verbosity;

struct Message {
  std::stringstream stream_;
  int verbosity_;

  Message(Verbosity verbosity, const char* file, int line);
  ~Message();
};
}

#define LOG_IF(v, cond)            \
  !(cond) ? void(0)                \
          : ccls::log::Voidify() & \
                ccls::log::Message(v, __FILE__, __LINE__).stream_
#define LOG_S(v)                   \
  LOG_IF(ccls::log::Verbosity_##v, \
         ccls::log::Verbosity_##v <= ccls::log::verbosity)
#define LOG_IF_S(v, cond)          \
  LOG_IF(ccls::log::Verbosity_##v, \
         (cond) && ccls::log::Verbosity_##v <= ccls::log::verbosity)
#define CHECK_S(cond) LOG_IF(FATAL, !(cond)) << "check failed: " #cond " "
