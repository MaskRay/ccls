// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "config.hh"

#include <algorithm>
#include <thread>

namespace ccls {
Config *g_config;

void doPathMapping(std::string &arg) {
  for (const std::string &mapping : g_config->clang.pathMappings) {
    auto sep = mapping.find('>');
    if (sep != std::string::npos) {
      auto p = arg.find(mapping.substr(0, sep));
      if (p != std::string::npos)
        arg.replace(p, sep, mapping.substr(sep + 1));
    }
  }
}

int defaultIndexThreadCount() {
  constexpr float kIndexThreadUtilization = 0.8;
  return std::max(1, static_cast<int>(std::thread::hardware_concurrency() *
                                      kIndexThreadUtilization));
}
} // namespace ccls
