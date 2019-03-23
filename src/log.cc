/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

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
    SmallString<32> Name;
    get_thread_name(Name);
    stream_ << std::left << std::setw(13) << Name.c_str();
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
    case Verbosity::FATAL: stream_ << 'F'; break;
    case Verbosity::ERROR: stream_ << 'E'; break;
    case Verbosity::WARNING: stream_ << 'W'; break;
    case Verbosity::INFO: stream_ << 'I'; break;
    case Verbosity::DEBUG: stream_ << 'D'; break;
    case Verbosity::VERBOSE: stream_ << 'V'; break;
    default: stream_ << "V(" << +verbosity_ << ')';
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
  if (verbosity_ == Verbosity::FATAL)
    abort();
}
} // namespace ccls::log
