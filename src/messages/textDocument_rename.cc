// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.hh"
#include "query_utils.hh"

namespace ccls {
namespace {
WorkspaceEdit BuildWorkspaceEdit(DB *db, WorkingFiles *wfiles, SymbolRef sym,
                                 const std::string &new_text) {
  std::unordered_map<int, TextDocumentEdit> path_to_edit;

  EachOccurrence(db, sym, true, [&](Use use) {
    std::optional<Location> ls_location = GetLsLocation(db, wfiles, use);
    if (!ls_location)
      return;

    int file_id = use.file_id;
    if (path_to_edit.find(file_id) == path_to_edit.end()) {
      path_to_edit[file_id] = TextDocumentEdit();

      QueryFile &file = db->files[file_id];
      if (!file.def)
        return;

      const std::string &path = file.def->path;
      path_to_edit[file_id].textDocument.uri = DocumentUri::FromPath(path);

      WorkingFile *working_file = wfiles->GetFileByFilename(path);
      if (working_file)
        path_to_edit[file_id].textDocument.version = working_file->version;
    }

    TextEdit edit;
    edit.range = ls_location->range;
    edit.newText = new_text;

    // vscode complains if we submit overlapping text edits.
    auto &edits = path_to_edit[file_id].edits;
    if (std::find(edits.begin(), edits.end(), edit) == edits.end())
      edits.push_back(edit);
  });

  WorkspaceEdit edit;
  for (const auto &changes : path_to_edit)
    edit.documentChanges.push_back(changes.second);
  return edit;
}
} // namespace

void MessageHandler::textDocument_rename(RenameParam &param, ReplyOnce &reply) {
  int file_id;
  QueryFile *file = FindFile(reply, param.textDocument.uri.GetPath(), &file_id);
  if (!file)
    return;

  WorkingFile *wfile = wfiles->GetFileByFilename(file->def->path);
  WorkspaceEdit result;
  for (SymbolRef sym : FindSymbolsAtLocation(wfile, file, param.position)) {
    result = BuildWorkspaceEdit(db, wfiles, sym, param.newName);
    break;
  }

  reply(result);
}
} // namespace ccls
