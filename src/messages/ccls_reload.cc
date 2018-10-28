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

#include "clang_complete.hh"
#include "match.h"
#include "message_handler.hh"
#include "pipeline.hh"
#include "project.hh"
#include "working_files.h"

#include <queue>
#include <unordered_set>

namespace ccls {
namespace {
struct Param {
  bool dependencies = true;
  std::vector<std::string> whitelist;
  std::vector<std::string> blacklist;
};
MAKE_REFLECT_STRUCT(Param, dependencies, whitelist, blacklist);
} // namespace

void MessageHandler::ccls_reload(Reader &reader) {
  Param param;
  Reflect(reader, param);
    // Send index requests for every file.
  if (param.whitelist.empty() && param.blacklist.empty()) {
    vfs->Clear();
    db->clear();
    project->Index(working_files, lsRequestId());
    clang_complete->FlushAllSessions();
    return;
  }
}
} // namespace ccls
