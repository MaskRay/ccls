#include "working_files.h"

WorkingFile::WorkingFile(const std::string& filename, const std::string& content)
  : filename(filename), content(content) {
}

CXUnsavedFile WorkingFile::AsUnsavedFile() const {
  CXUnsavedFile result;
  result.Filename = filename.c_str();
  result.Contents = content.c_str();
  result.Length = content.size();
  return result;
}

WorkingFile* WorkingFiles::GetFileByFilename(const std::string& filename) {
  for (auto& file : files) {
    if (file->filename == filename)
      return file.get();
  }
  return nullptr;
}

void WorkingFiles::OnOpen(const Ipc_TextDocumentDidOpen::Params& open) {
  std::string filename = open.textDocument.uri.GetPath();
  std::string content = open.textDocument.text;

  // The file may already be open.
  if (WorkingFile* file = GetFileByFilename(filename)) {
    file->content = content;
    return;
  }

  files.push_back(MakeUnique<WorkingFile>(filename, content));
}

void WorkingFiles::OnChange(const Ipc_TextDocumentDidChange::Params& change) {
  std::string filename = change.textDocument.uri.GetPath();
  WorkingFile* file = GetFileByFilename(filename);
  if (!file) {
    std::cerr << "Could not change " << filename << " because it was not open" << std::endl;
    return;
  }

  // TODO: we should probably pay attention to versioning.

  for (const Ipc_TextDocumentDidChange::lsTextDocumentContentChangeEvent& diff : change.contentChanges) {
    // If range or rangeLength are emitted we replace everything, per the spec.
    if (diff.rangeLength == -1) {
      file->content = diff.text;
    }
    else {
      file->content.replace(file->content.begin(), file->content.begin() + diff.rangeLength, diff.text);
    }
  }
}

void WorkingFiles::OnClose(const Ipc_TextDocumentDidClose::Params& close) {
  std::string filename = close.textDocument.uri.GetPath();

  for (int i = 0; i < files.size(); ++i) {
    if (files[i]->filename == filename) {
      files.erase(files.begin() + i);
      return;
    }
  }

  std::cerr << "Could not close " << filename << " because it was not open" << std::endl;
}

std::vector<CXUnsavedFile> WorkingFiles::AsUnsavedFiles() const {
  std::vector<CXUnsavedFile> result;
  result.reserve(files.size());
  for (auto& file : files)
    result.push_back(file->AsUnsavedFile());
  return result;
}

