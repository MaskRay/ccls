// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "log.hh"

#include <llvm/ADT/SmallString.h>
#include <llvm/Support/Threading.h>

#include <iomanip>
#include <mutex>
#include <stdlib.h>
#include <string.h>
#include <time.h>

namespace ccls::log {
static std::mutex mtx;
FILE *file;
Verbosity verbosity;

Message::Message(Verbosity verbosity, const char *file, int line)
    : verbosity_(verbosity) {
  using namespace llvm;
  time_t tim = time(NULL);
  struct tm t;
  {
    std::lock_guard<std::mutex> lock(mtx);
    t = *localtime(&tim);
  }
  char buf[16];
  snprintf(buf, sizeof buf, "%02d:%02d:%02d ", t.tm_hour, t.tm_min, t.tm_sec);
  stream_ << buf;
  {
    SmallString<32> name;
    get_thread_name(name);
    stream_ << std::left << std::setw(13) << name.c_str();
  }
  {
    const char *p = strrchr(file, '/');
    if (p)
      file = p + 1;
    stream_ << std::right << std::setw(15) << file << ':' << std::left
            << std::setw(3) << line;
  }
  stream_ << ' ';
  // clang-format off
  switch (verbosity_) {
    case Verbosity_FATAL: stream_ << 'F'; break;
    case Verbosity_ERROR: stream_ << 'E'; break;
    case Verbosity_WARNING: stream_ << 'W'; break;
    case Verbosity_INFO: stream_ << 'I'; break;
    default: stream_ << "V(" << int(verbosity_) << ')';
  }
  // clang-format on
  stream_ << ' ';
}

Message::~Message() {
  if (!file)
    return;
  std::lock_guard<std::mutex> lock(mtx);
  stream_ << '\n';
  fputs(stream_.str().c_str(), file);
  fflush(file);
  if (verbosity_ == Verbosity_FATAL)
    abort();
}
} // namespace ccls::log
