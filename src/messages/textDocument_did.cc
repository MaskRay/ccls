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

#include "include_complete.hh"
#include "message_handler.hh"
#include "pipeline.hh"
#include "project.hh"
#include "sema_manager.hh"
#include "working_files.hh"

namespace ccls {
void MessageHandler::textDocument_didChange(TextDocumentDidChangeParam &param) {
  std::string path = param.textDocument.uri.GetPath();
  wfiles->OnChange(param);
  if (g_config->index.onChange)
    pipeline::Index(path, {}, IndexMode::OnChange, true);
  manager->OnView(path);
  if (g_config->diagnostics.onChange >= 0)
    manager->ScheduleDiag(path, g_config->diagnostics.onChange);
}

void MessageHandler::textDocument_didClose(TextDocumentParam &param) {
  std::string path = param.textDocument.uri.GetPath();
  wfiles->OnClose(path);
  manager->OnClose(path);
}

void MessageHandler::textDocument_didOpen(DidOpenTextDocumentParam &param) {
  std::string path = param.textDocument.uri.GetPath();
  WorkingFile *wf = wfiles->OnOpen(param.textDocument);
  if (std::optional<std::string> cached_file_contents =
          pipeline::LoadIndexedContent(path))
    wf->SetIndexContent(*cached_file_contents);

  QueryFile *file = FindFile(path);
  if (file) {
    EmitSkippedRanges(wf, *file);
    EmitSemanticHighlight(db, wf, *file);
  }
  include_complete->AddFile(wf->filename);

  // Submit new index request if it is not a header file or there is no
  // pending index request.
  auto [lang, header] = lookupExtension(path);
  if ((lang != LanguageId::Unknown && !header) ||
      !pipeline::pending_index_requests)
    pipeline::Index(path, {}, IndexMode::Normal, false);
  if (header)
    project->IndexRelated(path);

  manager->OnView(path);
}

void MessageHandler::textDocument_didSave(TextDocumentParam &param) {
  const std::string &path = param.textDocument.uri.GetPath();
  pipeline::Index(path, {}, IndexMode::Normal, true);
  manager->OnSave(path);
}
} // namespace ccls
