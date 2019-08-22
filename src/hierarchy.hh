// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "lsp.hh"

#include <algorithm>
#include <queue>

namespace ccls {
template <typename Node>
std::vector<Location> flattenHierarchy(const std::optional<Node> &root) {
  if (!root)
    return {};
  std::vector<Location> ret;
  std::queue<const Node *> q;
  for (auto &entry : root->children)
    q.push(&entry);
  while (q.size()) {
    auto *entry = q.front();
    q.pop();
    if (entry->location.uri.raw_uri.size())
      ret.push_back({entry->location});
    for (auto &entry1 : entry->children)
      q.push(&entry1);
  }
  std::sort(ret.begin(), ret.end());
  ret.erase(std::unique(ret.begin(), ret.end()), ret.end());
  return ret;
}
} // namespace ccls
