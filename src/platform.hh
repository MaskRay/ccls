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

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace ccls {
std::string NormalizePath(const std::string &path);

// Free any unused memory and return it to the system.
void FreeUnusedMemory();

// Stop self and wait for SIGCONT.
void TraceMe();

void SpawnThread(void *(*fn)(void *), void *arg);
} // namespace ccls
