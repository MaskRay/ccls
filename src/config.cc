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

#include "config.hh"

namespace ccls {
Config *g_config;

void DoPathMapping(std::string &arg) {
  for (const std::string &mapping : g_config->clang.pathMappings) {
    auto colon = mapping.find('>');
    if (colon != std::string::npos) {
        auto p = arg.find(mapping.substr(0, colon));
        if (p != std::string::npos)
            arg.replace(p, colon, mapping.substr(colon + 1));
    } else {
      // Deprecated: Use only for older settings
      auto colon = mapping.find(':');
      if (colon != std::string::npos) {
        auto p = arg.find(mapping.substr(0, colon));
        if (p != std::string::npos)
          arg.replace(p, colon, mapping.substr(colon + 1));
      }
    }
  }
}
}
