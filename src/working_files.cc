#include "working_files.h"

#include "position.h"

namespace {

int GetOffsetForPosition(lsPosition position, const std::string& content) {
  int offset = 0;

  int remaining_lines = position.line;
  while (remaining_lines > 0) {
    if (content[offset] == '\n')
      --remaining_lines;
    ++offset;
  }

  return offset + position.character;
}

}  // namespace

WorkingFile::WorkingFile(const std::string& filename, const std::string& buffer_content)
    : filename(filename), buffer_content(buffer_content) {
  OnBufferContentUpdated();
  // TODO: use cached index file contents
  SetIndexContent(buffer_content);
}

void WorkingFile::SetIndexContent(const std::string& index_content) {
  index_lines = ToLines(index_content, true /*trim_whitespace*/);
}

void WorkingFile::OnBufferContentUpdated() {
  all_buffer_lines = ToLines(buffer_content, true /*trim_whitespace*/);

  // Build lookup buffer.
  buffer_line_lookup.clear();
  buffer_line_lookup.reserve(all_buffer_lines.size());
  for (int i = 0; i < all_buffer_lines.size(); ++i) {
    const std::string& buffer_line = all_buffer_lines[i];

    auto it = buffer_line_lookup.find(buffer_line);
    if (it == buffer_line_lookup.end())
      buffer_line_lookup[buffer_line] = { i + 1 };
    else
      it->second.push_back(i + 1);
  }
}

optional<int> WorkingFile::GetBufferLineFromDiskLine(int index_line) const {
  // The implementation is simple but works pretty well for most cases. We
  // lookup the line contents in the indexed file contents, and try to find the
  // most similar line in the current buffer file.
  //
  // Previously, this was implemented by tracking edits and by running myers
  // diff algorithm. They were complex implementations that did not work as
  // well.

  // Note: |index_line| and |buffer_line| are 1-based.

  assert(index_line >= 1 && index_line <= index_lines.size());

  // Find the line in the cached index file. We'll try to find the most similar line
  // in the buffer and return the index for that.
  std::string index = index_lines[index_line - 1];
  auto buffer_it = buffer_line_lookup.find(index);
  if (buffer_it == buffer_line_lookup.end()) {
    // TODO: Use levenshtein distance to find the best match (but only to an
    // extent)
    return nullopt;
  }

  // From all the identical lines, return the one which is closest to
  // |index_line|. There will usually only be one identical line.
  assert(!buffer_it->second.empty());
  int closest_dist = INT_MAX;
  int closest_buffer_line = INT_MIN;
  for (int buffer_line : buffer_it->second) {
    int dist = std::abs(buffer_line - index_line);
    if (dist <= closest_dist) {
      closest_dist = dist;
      closest_buffer_line = buffer_line;
    }
  }

  return closest_buffer_line;
}

CXUnsavedFile WorkingFile::AsUnsavedFile() const {
  CXUnsavedFile result;
  result.Filename = filename.c_str();
  result.Contents = buffer_content.c_str();
  result.Length = (unsigned long)buffer_content.size();
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
    file->version = open.textDocument.version;
    // TODO: Load saved indexed content and not the initial buffer content.
    file->SetIndexContent(content);
    file->buffer_content = content;
    file->OnBufferContentUpdated();
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

  file->version = change.textDocument.version;
  //std::cerr << "VERSION " << change.textDocument.version << std::endl;

  for (const Ipc_TextDocumentDidChange::lsTextDocumentContentChangeEvent& diff : change.contentChanges) {
    //std::cerr << "Applying rangeLength=" << diff.rangeLength;

    // If range or rangeLength are emitted we replace everything, per the spec.
    if (diff.rangeLength == -1) {
      file->buffer_content = diff.text;
      file->OnBufferContentUpdated();
    }
    else {
      int start_offset = GetOffsetForPosition(diff.range.start, file->buffer_content);
      file->buffer_content.replace(file->buffer_content.begin() + start_offset,
        file->buffer_content.begin() + start_offset + diff.rangeLength,
        diff.text);
      file->OnBufferContentUpdated();
    }
  }
  //std::cerr << std::endl << std::endl << "--------" << file->content << "--------" << std::endl << std::endl;
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