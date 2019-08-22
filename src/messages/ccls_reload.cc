// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.hh"
#include "pipeline.hh"
#include "project.hh"
#include "sema_manager.hh"
#include "working_files.hh"

#include <queue>
#include <unordered_set>

namespace ccls {
namespace {
struct Param {
  bool dependencies = true;
  std::vector<std::string> whitelist;
  std::vector<std::string> blacklist;
};
REFLECT_STRUCT(Param, dependencies, whitelist, blacklist);
} // namespace

void MessageHandler::ccls_reload(JsonReader &reader) {
  Param param;
  reflect(reader, param);
  // Send index requests for every file.
  if (param.whitelist.empty() && param.blacklist.empty()) {
    vfs->clear();
    db->clear();
    project->index(wfiles, RequestId());
    manager->clear();
    return;
  }
}
} // namespace ccls
