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

#include "filesystem.hh"
using namespace llvm;

#include "utils.hh"

#include <set>
#include <vector>

void getFilesInFolder(std::string folder, bool recursive, bool dir_prefix,
                      const std::function<void(const std::string &)> &handler) {
  ccls::ensureEndsInSlash(folder);
  sys::fs::file_status status;
  if (sys::fs::status(folder, status, true))
    return;
  sys::fs::UniqueID id;
  std::vector<std::string> curr{folder};
  std::vector<std::pair<std::string, sys::fs::file_status>> succ;
  std::set<sys::fs::UniqueID> seen{status.getUniqueID()};
  while (curr.size() || succ.size()) {
    if (curr.empty()) {
      for (auto &it : succ)
        if (!seen.count(it.second.getUniqueID()))
          curr.push_back(std::move(it.first));
      succ.clear();
    } else {
      std::error_code ec;
      std::string folder1 = curr.back();
      curr.pop_back();
      for (sys::fs::directory_iterator i(folder1, ec, false), e; i != e && !ec;
           i.increment(ec)) {
        std::string path = i->path(), filename = sys::path::filename(path);
        if ((filename[0] == '.' && filename != ".ccls") ||
            sys::fs::status(path, status, false))
          continue;
        if (sys::fs::is_symlink_file(status)) {
          if (sys::fs::status(path, status, true))
            continue;
          if (sys::fs::is_directory(status)) {
            if (recursive)
              succ.emplace_back(path, status);
            continue;
          }
        }
        if (sys::fs::is_regular_file(status)) {
          if (!dir_prefix)
            path = path.substr(folder.size());
          handler(sys::path::convert_to_slash(path));
        } else if (recursive && sys::fs::is_directory(status) &&
                   !seen.count(id = status.getUniqueID())) {
          curr.push_back(path);
          seen.insert(id);
        }
      }
    }
  }
}
