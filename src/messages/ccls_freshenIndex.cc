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
MethodType kMethodType = "$ccls/freshenIndex";

struct In_CclsFreshenIndex : public NotificationInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct Params {
    bool dependencies = true;
    std::vector<std::string> whitelist;
    std::vector<std::string> blacklist;
  };
  Params params;
};
MAKE_REFLECT_STRUCT(In_CclsFreshenIndex::Params, dependencies, whitelist,
                    blacklist);
MAKE_REFLECT_STRUCT(In_CclsFreshenIndex, params);
REGISTER_IN_MESSAGE(In_CclsFreshenIndex);

struct Handler_CclsFreshenIndex : BaseMessageHandler<In_CclsFreshenIndex> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_CclsFreshenIndex *request) override {
    GroupMatch matcher(request->params.whitelist, request->params.blacklist);

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

      std::optional<int64_t> write_time = LastWriteTime(file->def->path);
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

    // Send index requests for every file.
    project->Index(working_files, lsRequestId());
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CclsFreshenIndex);
} // namespace
