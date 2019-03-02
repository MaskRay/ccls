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

#include "message_handler.hh"
#include "query.hh"

#include <clang/Basic/CharInfo.h>

#include <unordered_set>

using namespace clang;

namespace ccls {
namespace {
WorkspaceEdit BuildWorkspaceEdit(DB *db, WorkingFiles *wfiles, SymbolRef sym,
                                 std::string_view old_text,
                                 const std::string &new_text) {
  std::unordered_map<int, std::pair<WorkingFile *, TextDocumentEdit>> path2edit;
  std::unordered_map<int, std::unordered_set<Range>> edited;

  EachOccurrence(db, sym, true, [&](Use use) {
    int file_id = use.file_id;
    QueryFile &file = db->files[file_id];
    if (!file.def || !edited[file_id].insert(use.range).second)
      return;
    std::optional<Location> loc = GetLsLocation(db, wfiles, use);
    if (!loc)
      return;

    auto [it, inserted] = path2edit.try_emplace(file_id);
    auto &edit = it->second.second;
    if (inserted) {
      const std::string &path = file.def->path;
      edit.textDocument.uri = DocumentUri::FromPath(path);
      if ((it->second.first = wfiles->GetFile(path)))
        edit.textDocument.version = it->second.first->version;
    }
    // TODO LoadIndexedContent if wf is nullptr.
    if (WorkingFile *wf = it->second.first) {
      int start = GetOffsetForPosition(loc->range.start, wf->buffer_content),
          end = GetOffsetForPosition(loc->range.end, wf->buffer_content);
      if (wf->buffer_content.compare(start, end - start, old_text))
        return;
    }
    edit.edits.push_back({loc->range, new_text});
  });

  WorkspaceEdit ret;
  for (auto &x : path2edit)
    ret.documentChanges.push_back(std::move(x.second.second));
  return ret;
}
} // namespace

void MessageHandler::textDocument_rename(RenameParam &param, ReplyOnce &reply) {
  auto [file, wf] = FindOrFail(param.textDocument.uri.GetPath(), reply);
  if (!wf)
    return;
  WorkspaceEdit result;

  for (SymbolRef sym : FindSymbolsAtLocation(wf, file, param.position)) {
    result = BuildWorkspaceEdit(
        db, wfiles, sym,
        LexIdentifierAroundPos(param.position, wf->buffer_content),
        param.newName);
    break;
  }

  reply(result);
}
} // namespace ccls
