#pragma once

#include "language_server_api.h"

#include <clang-c/Index.h>

#include <string>

struct WorkingFile {
  std::string filename;
  std::string content;

  WorkingFile(const std::string& filename, const std::string& content);

  CXUnsavedFile AsUnsavedFile() const;
};

struct WorkingFiles {
  // Find the file with the given filename.
  WorkingFile* GetFileByFilename(const std::string& filename);
  void OnOpen(const Ipc_TextDocumentDidOpen::Params& open);
  void OnChange(const Ipc_TextDocumentDidChange::Params& change);
  void OnClose(const Ipc_TextDocumentDidClose::Params& close);

  std::vector<CXUnsavedFile> AsUnsavedFiles() const;

  // Use unique_ptrs so we can handout WorkingFile ptrs and not have them
  // invalidated if we resize files.
  std::vector<std::unique_ptr<WorkingFile>> files;
};