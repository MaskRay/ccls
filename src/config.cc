// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "config.h"

Config *g_config;
thread_local int g_thread_id;

namespace ccls {
void DoPathMapping(std::string &arg) {
  for (const std::string &mapping : g_config->clang.pathMappings) {
    auto sep = mapping.find('>');
    if (sep != std::string::npos) {
      auto p = arg.find(mapping.substr(0, sep));
      if (p != std::string::npos)
        arg.replace(p, sep, mapping.substr(sep + 1));
    }
  }
}
}
