// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "match.h"
#include "message_handler.h"
#include "pipeline.hh"
#include "platform.h"
#include "project.h"
#include "working_files.h"
using namespace ccls;

#include <queue>
#include <unordered_set>

namespace {
MethodType kMethodType = "$ccls/reload";

struct In_CclsReload : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct Params {
    bool dependencies = true;
    std::vector<std::string> whitelist;
    std::vector<std::string> blacklist;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(In_CclsReload::Params, dependencies, whitelist,
                    blacklist);
MAKE_REFLECT_STRUCT(In_CclsReload, params);
REGISTER_IN_MESSAGE(In_CclsReload);

struct Handler_CclsReload : BaseMessageHandler<In_CclsReload> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_CclsReload *request) override {
    const auto &params = request->params;
    // Send index requests for every file.
    if (params.whitelist.empty() && params.blacklist.empty()) {
      {
        std::lock_guard lock(vfs->mutex);
        vfs->state.clear();
      }
      db->clear();
      project->Index(working_files, lsRequestId());
      return;
    }

    // TODO
    GroupMatch matcher(params.whitelist, params.blacklist);

    std::queue<const QueryFile *> q;
    // |need_index| stores every filename ever enqueued.
    std::unordered_set<std::string> need_index;
    // Reverse dependency graph.
    std::unordered_map<std::string, std::vector<std::string>> graph;
    // filename -> QueryFile mapping for files haven't enqueued.
    std::unordered_map<std::string, const QueryFile *> path_to_file;

    for (const auto &file : db->files)
      if (file.def) {
        if (matcher.IsMatch(file.def->path))
          q.push(&file);
        else
          path_to_file[file.def->path] = &file;
        for (const std::string &dependency : file.def->dependencies)
          graph[dependency].push_back(file.def->path);
      }

    while (!q.empty()) {
      const QueryFile *file = q.front();
      q.pop();
      need_index.insert(file->def->path);

      std::optional<int64_t> write_time =
          pipeline::LastWriteTime(file->def->path);
      if (!write_time)
        continue;
      {
        std::lock_guard<std::mutex> lock(vfs->mutex);
        VFS::State &st = vfs->state[file->def->path];
        if (st.timestamp < write_time)
          st.stage = 0;
      }

      if (request->params.dependencies)
        for (const std::string &path : graph[file->def->path]) {
          auto it = path_to_file.find(path);
          if (it != path_to_file.end()) {
            q.push(it->second);
            path_to_file.erase(it);
          }
        }
    }
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CclsReload);
} // namespace
