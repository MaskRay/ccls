// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "clang_complete.hh"
#include "match.hh"
#include "message_handler.hh"
#include "pipeline.hh"
#include "project.hh"
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
MAKE_REFLECT_STRUCT(Param, dependencies, whitelist, blacklist);
} // namespace

void MessageHandler::ccls_reload(Reader &reader) {
  Param param;
  Reflect(reader, param);
    // Send index requests for every file.
  if (param.whitelist.empty() && param.blacklist.empty()) {
    vfs->Clear();
    db->clear();
    project->Index(wfiles, lsRequestId());
    clang_complete->FlushAllSessions();
    return;
  }
}
} // namespace ccls
