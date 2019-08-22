// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.hh"
#include "query.hh"

#include <clang/Basic/CharInfo.h>

#include <unordered_set>

using namespace clang;

namespace ccls {
namespace {
WorkspaceEdit buildWorkspaceEdit(DB *db, WorkingFiles *wfiles, SymbolRef sym,
                                 std::string_view old_text,
                                 const std::string &new_text) {
  std::unordered_map<int, std::pair<WorkingFile *, TextDocumentEdit>> path2edit;
  std::unordered_map<int, std::unordered_set<Range>> edited;

  eachOccurrence(db, sym, true, [&](Use use) {
    int file_id = use.file_id;
    QueryFile &file = db->files[file_id];
    if (!file.def || !edited[file_id].insert(use.range).second)
      return;
    std::optional<Location> loc = getLsLocation(db, wfiles, use);
    if (!loc)
      return;

    auto [it, inserted] = path2edit.try_emplace(file_id);
    auto &edit = it->second.second;
    if (inserted) {
      const std::string &path = file.def->path;
      edit.textDocument.uri = DocumentUri::fromPath(path);
      if ((it->second.first = wfiles->getFile(path)))
        edit.textDocument.version = it->second.first->version;
    }
    // TODO LoadIndexedContent if wf is nullptr.
    if (WorkingFile *wf = it->second.first) {
      int start = getOffsetForPosition(loc->range.start, wf->buffer_content),
          end = getOffsetForPosition(loc->range.end, wf->buffer_content);
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
  auto [file, wf] = findOrFail(param.textDocument.uri.getPath(), reply);
  if (!wf)
    return;
  WorkspaceEdit result;

  for (SymbolRef sym : findSymbolsAtLocation(wf, file, param.position)) {
    result = buildWorkspaceEdit(
        db, wfiles, sym,
        lexIdentifierAroundPos(param.position, wf->buffer_content),
        param.newName);
    break;
  }

  reply(result);
}
} // namespace ccls
