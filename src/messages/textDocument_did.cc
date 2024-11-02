// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.hh"
#include "pipeline.hh"
#include "project.hh"
#include "sema_manager.hh"
#include "working_files.hh"

namespace ccls {
void MessageHandler::textDocument_didChange(TextDocumentDidChangeParam &param) {
  std::string path = param.textDocument.uri.getPath();
  wfiles->onChange(param);
  if (g_config->index.onChange)
    pipeline::index(path, {}, IndexMode::OnChange, true);
  manager->onView(path);
  if (g_config->diagnostics.onChange >= 0)
    manager->scheduleDiag(path, g_config->diagnostics.onChange);
}

void MessageHandler::textDocument_didClose(TextDocumentParam &param) {
  std::string path = param.textDocument.uri.getPath();
  wfiles->onClose(path);
  manager->onClose(path);
  pipeline::removeCache(path);
}

void MessageHandler::textDocument_didOpen(DidOpenTextDocumentParam &param) {
  std::string path = param.textDocument.uri.getPath();
  WorkingFile *wf = wfiles->onOpen(param.textDocument);
  if (std::optional<std::string> cached_file_contents =
          pipeline::loadIndexedContent(path))
    wf->setIndexContent(*cached_file_contents);

  QueryFile *file = findFile(path);
  if (file) {
    emitSkippedRanges(wf, *file);
    emitSemanticHighlight(db, wf, *file);
  }

  // Submit new index request if it is not a header file or there is no
  // pending index request.
  auto [lang, header] = lookupExtension(path);
  if ((lang != LanguageId::Unknown && !header) ||
      pipeline::stats.completed == pipeline::stats.enqueued)
    pipeline::index(path, {}, IndexMode::Normal, false);
  if (header)
    project->indexRelated(path);

  manager->onView(path);

  // For the first few didOpen, sort indexer requests based on path similarity.
  if (++pipeline::stats.opened >= 5)
    return;
  std::unordered_map<std::string, int> dir2prio;
  {
    std::lock_guard lock(wfiles->mutex);
    for (auto &[f, wf] : wfiles->files) {
      std::string cur = lowerPathIfInsensitive(f);
      for (int pri = 1 << 20; !(cur = llvm::sys::path::parent_path(cur)).empty(); pri /= 2)
        dir2prio[cur] += pri;
    }
  }
  pipeline::indexerSort(dir2prio);
}

void MessageHandler::textDocument_didSave(TextDocumentParam &param) {
  const std::string &path = param.textDocument.uri.getPath();
  pipeline::index(path, {}, IndexMode::Normal, false);
  manager->onSave(path);
}
} // namespace ccls
