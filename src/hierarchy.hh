// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "lsp.h"

#include <algorithm>
#include <queue>

template <typename Node>
void FlattenHierarchy(const Node &root, Out_LocationList &out) {
  std::queue<const Node *> q;
  for (auto &entry : root.children)
    q.push(&entry);
  while (q.size()) {
    auto *entry = q.front();
    q.pop();
    if (entry->location.uri.raw_uri.size())
      out.result.push_back({entry->location});
    for (auto &entry1 : entry->children)
      q.push(&entry1);
  }
  std::sort(out.result.begin(), out.result.end());
  out.result.erase(std::unique(out.result.begin(), out.result.end()),
    out.result.end());
}
