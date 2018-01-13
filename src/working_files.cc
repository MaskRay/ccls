#include "working_files.h"

#include "lex_utils.h"
#include "position.h"

#include <doctest/doctest.h>
#include <loguru.hpp>

#include <climits>

namespace {

lsPosition GetPositionForOffset(const std::string& content, int offset) {
  if (offset >= content.size())
    offset = (int)content.size() - 1;

  lsPosition result;
  int i = 0;
  while (i < offset) {
    if (content[i] == '\n') {
      result.line += 1;
      result.character = 0;
    } else {
      result.character += 1;
    }
    ++i;
  }

  return result;
}

}  // namespace

std::vector<CXUnsavedFile> WorkingFiles::Snapshot::AsUnsavedFiles() const {
  std::vector<CXUnsavedFile> result;
  result.reserve(files.size());
  for (auto& file : files) {
    CXUnsavedFile unsaved;
    unsaved.Filename = file.filename.c_str();
    unsaved.Contents = file.content.c_str();
    unsaved.Length = (unsigned long)file.content.size();

    result.push_back(unsaved);
  }
  return result;
}

WorkingFile::WorkingFile(const std::string& filename,
                         const std::string& buffer_content)
    : filename(filename), buffer_content(buffer_content) {
  OnBufferContentUpdated();

  // SetIndexContent gets called when the file is opened.
}

void WorkingFile::SetIndexContent(const std::string& index_content) {
  index_lines = ToLines(index_content, false /*trim_whitespace*/);

  // Build lookup buffer.
  index_lines_lookup.clear();
  index_lines_lookup.reserve(index_lines.size());
  for (int i = 0; i < index_lines.size(); ++i) {
    std::string index_line = Trim(index_lines[i]);

    auto it = index_lines_lookup.find(index_line);
    if (it == index_lines_lookup.end())
      index_lines_lookup[index_line] = {i + 1};
    else
      it->second.push_back(i + 1);
  }
}

void WorkingFile::OnBufferContentUpdated() {
  all_buffer_lines = ToLines(buffer_content, true /*trim_whitespace*/);
  raw_buffer_lines = ToLines(buffer_content, false /*trim_whitespace*/);

  // Build lookup buffer.
  all_buffer_lines_lookup.clear();
  all_buffer_lines_lookup.reserve(all_buffer_lines.size());
  for (int i = 0; i < all_buffer_lines.size(); ++i) {
    const std::string& buffer_line = all_buffer_lines[i];

    auto it = all_buffer_lines_lookup.find(buffer_line);
    if (it == all_buffer_lines_lookup.end())
      all_buffer_lines_lookup[buffer_line] = {i + 1};
    else
      it->second.push_back(i + 1);
  }
}

optional<int> WorkingFile::GetBufferLineFromIndexLine(int index_line) const {
  // The implementation is simple but works pretty well for most cases. We
  // lookup the line contents in the indexed file contents, and try to find the
  // most similar line in the current buffer file.
  //
  // Previously, this was implemented by tracking edits and by running myers
  // diff algorithm. They were complex implementations that did not work as
  // well.

  // Note: |index_line| and |buffer_line| are 1-based.

  // TODO: reenable this assert once we are using the real indexed file.
  // assert(index_line >= 1 && index_line <= index_lines.size());
  if (index_line < 1 || index_line > index_lines.size()) {
    loguru::Text stack = loguru::stacktrace();
    LOG_S(WARNING) << "Bad index_line (got " << index_line << ", expected [1, "
                   << index_lines.size() << "]) in " << filename
                   << stack.c_str();
    return nullopt;
  }

  // Find the line in the cached index file. We'll try to find the most similar
  // line in the buffer and return the index for that.
  std::string index = Trim(index_lines[index_line - 1]);
  auto buffer_it = all_buffer_lines_lookup.find(index);
  if (buffer_it == all_buffer_lines_lookup.end()) {
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

optional<int> WorkingFile::GetIndexLineFromBufferLine(int buffer_line) const {
  // See GetBufferLineFromIndexLine for additional comments.

  // Note: |index_line| and |buffer_line| are 1-based.
  // assert(buffer_line >= 1 && buffer_line < all_buffer_lines.size());
  if (buffer_line < 1 || buffer_line > all_buffer_lines.size()) {
    loguru::Text stack = loguru::stacktrace();
    LOG_S(WARNING) << "Bad buffer_line (got " << buffer_line
                   << ", expected [1, " << all_buffer_lines.size() << "]) in "
                   << filename << stack.c_str();
    return nullopt;
  }

  // Find the line in the index file. We'll try to find the most similar line
  // in the index file and return the index for that.
  std::string buffer = all_buffer_lines[buffer_line - 1];
  auto index_it = index_lines_lookup.find(buffer);
  if (index_it == index_lines_lookup.end()) {
    // TODO: Use levenshtein distance to find the best match (but only to an
    // extent)
    return nullopt;
  }

  // From all the identical lines, return the one which is closest to
  // |index_line|. There will usually only be one identical line.
  assert(!index_it->second.empty());
  int closest_dist = INT_MAX;
  int closest_index_line = INT_MIN;
  for (int index_line : index_it->second) {
    int dist = std::abs(buffer_line - index_line);
    if (dist <= closest_dist) {
      closest_dist = dist;
      closest_index_line = index_line;
    }
  }

  return closest_index_line;
}

optional<std::string> WorkingFile::GetBufferLineContentFromIndexLine(
    int indexed_line,
    optional<int>* out_buffer_line) const {
  optional<int> buffer_line = GetBufferLineFromIndexLine(indexed_line);
  if (out_buffer_line)
    *out_buffer_line = buffer_line;

  if (!buffer_line)
    return nullopt;

  if (*buffer_line < 1 || *buffer_line >= all_buffer_lines.size()) {
    LOG_S(WARNING) << "GetBufferLineContentFromIndexLine buffer line lookup not"
                   << " in all_buffer_lines";
    return nullopt;
  }

  return all_buffer_lines[*buffer_line - 1];
}

std::string WorkingFile::FindClosestCallNameInBuffer(
    lsPosition position,
    int* active_parameter,
    lsPosition* completion_position) const {
  *active_parameter = 0;

  int offset = GetOffsetForPosition(position, buffer_content);

  // If vscode auto-inserts closing ')' we will begin on ')' token in foo()
  // which will make the below algorithm think it's a nested call.
  if (offset > 0 && buffer_content[offset] == ')')
    --offset;

  // Scan back out of call context.
  int balance = 0;
  while (offset > 0) {
    char c = buffer_content[offset];
    if (c == ')')
      ++balance;
    else if (c == '(')
      --balance;

    if (balance == 0 && c == ',')
      *active_parameter += 1;

    --offset;

    if (balance == -1)
      break;
  }

  if (offset < 0)
    return "";

  // Scan back entire identifier.
  int start_offset = offset;
  while (offset > 0) {
    char c = buffer_content[offset - 1];
    if (isalnum(c) == false && c != '_')
      break;
    --offset;
  }

  if (completion_position)
    *completion_position = GetPositionForOffset(buffer_content, offset);

  return buffer_content.substr(offset, start_offset - offset + 1);
}

lsPosition WorkingFile::FindStableCompletionSource(
    lsPosition position,
    bool* is_global_completion,
    std::string* existing_completion) const {
  *is_global_completion = true;

  int start_offset = GetOffsetForPosition(position, buffer_content);
  int offset = start_offset;

  while (offset > 0) {
    char c = buffer_content[offset - 1];
    if (!isalnum(c) && c != '_') {
      // Global completion is everything except for dot (.), arrow (->), and
      // double colon (::)
      if (c == '.')
        *is_global_completion = false;
      if (offset > 2) {
        char pc = buffer_content[offset - 2];
        if (pc == ':' && c == ':')
          *is_global_completion = false;
        else if (pc == '-' && c == '>')
          *is_global_completion = false;
      }

      break;
    }
    --offset;
  }

  *existing_completion = buffer_content.substr(offset, start_offset - offset);
  return GetPositionForOffset(buffer_content, offset);
}

WorkingFile* WorkingFiles::GetFileByFilename(const std::string& filename) {
  std::lock_guard<std::mutex> lock(files_mutex);
  return GetFileByFilenameNoLock(filename);
}

WorkingFile* WorkingFiles::GetFileByFilenameNoLock(
    const std::string& filename) {
  for (auto& file : files) {
    if (file->filename == filename)
      return file.get();
  }
  return nullptr;
}

void WorkingFiles::DoAction(const std::function<void()>& action) {
  std::lock_guard<std::mutex> lock(files_mutex);
  action();
}

void WorkingFiles::DoActionOnFile(
    const std::string& filename,
    const std::function<void(WorkingFile* file)>& action) {
  std::lock_guard<std::mutex> lock(files_mutex);
  WorkingFile* file = GetFileByFilenameNoLock(filename);
  action(file);
}

WorkingFile* WorkingFiles::OnOpen(const lsTextDocumentItem& open) {
  std::lock_guard<std::mutex> lock(files_mutex);

  std::string filename = open.uri.GetPath();
  std::string content = open.text;

  // The file may already be open.
  if (WorkingFile* file = GetFileByFilenameNoLock(filename)) {
    file->version = open.version;
    file->buffer_content = content;
    file->OnBufferContentUpdated();
    return file;
  }

  files.push_back(MakeUnique<WorkingFile>(filename, content));
  return files[files.size() - 1].get();
}

void WorkingFiles::OnChange(const lsTextDocumentDidChangeParams& change) {
  std::lock_guard<std::mutex> lock(files_mutex);

  std::string filename = change.textDocument.uri.GetPath();
  WorkingFile* file = GetFileByFilenameNoLock(filename);
  if (!file) {
    LOG_S(WARNING) << "Could not change " << filename
                   << " because it was not open";
    return;
  }

  // version: number | null
  if (std::holds_alternative<int>(change.textDocument.version))
    file->version = std::get<int>(change.textDocument.version);

  for (const lsTextDocumentContentChangeEvent& diff : change.contentChanges) {
    // Per the spec replace everything if the rangeLength and range are not set.
    // See https://github.com/Microsoft/language-server-protocol/issues/9.
    if (!diff.range) {
      file->buffer_content = diff.text;
      file->OnBufferContentUpdated();
    } else {
      int start_offset =
          GetOffsetForPosition(diff.range->start, file->buffer_content);
      // Ignore TextDocumentContentChangeEvent.rangeLength which causes trouble
      // when UTF-16 surrogate pairs are used.
      int end_offset = GetOffsetForPosition(diff.range->end, file->buffer_content);
      file->buffer_content.replace(file->buffer_content.begin() + start_offset,
                                   file->buffer_content.begin() + end_offset,
                                   diff.text);
      file->OnBufferContentUpdated();
    }
  }
}

void WorkingFiles::OnClose(const lsTextDocumentIdentifier& close) {
  std::lock_guard<std::mutex> lock(files_mutex);

  std::string filename = close.uri.GetPath();

  for (int i = 0; i < files.size(); ++i) {
    if (files[i]->filename == filename) {
      files.erase(files.begin() + i);
      return;
    }
  }

  LOG_S(WARNING) << "Could not close " << filename
                 << " because it was not open";
}

WorkingFiles::Snapshot WorkingFiles::AsSnapshot(
    const std::vector<std::string>& filter_paths) {
  std::lock_guard<std::mutex> lock(files_mutex);

  Snapshot result;
  result.files.reserve(files.size());
  for (const auto& file : files) {
    if (filter_paths.empty() || FindAnyPartial(file->filename, filter_paths))
      result.files.push_back({file->filename, file->buffer_content});
  }
  return result;
}

lsPosition CharPos(const WorkingFile& file,
                   char character,
                   int character_offset = 0) {
  return CharPos(file.buffer_content, character, character_offset);
}

TEST_SUITE("WorkingFile") {
  TEST_CASE("simple call") {
    WorkingFile f("foo.cc", "abcd(1, 2");
    int active_param = 0;
    REQUIRE(f.FindClosestCallNameInBuffer(CharPos(f, '('), &active_param) ==
            "abcd");
    REQUIRE(active_param == 0);
    REQUIRE(f.FindClosestCallNameInBuffer(CharPos(f, '1'), &active_param) ==
            "abcd");
    REQUIRE(active_param == 0);
    REQUIRE(f.FindClosestCallNameInBuffer(CharPos(f, ','), &active_param) ==
            "abcd");
    REQUIRE(active_param == 1);
    REQUIRE(f.FindClosestCallNameInBuffer(CharPos(f, ' '), &active_param) ==
            "abcd");
    REQUIRE(active_param == 1);
    REQUIRE(f.FindClosestCallNameInBuffer(CharPos(f, '2'), &active_param) ==
            "abcd");
    REQUIRE(active_param == 1);
  }

  TEST_CASE("nested call") {
    WorkingFile f("foo.cc", "abcd(efg(), 2");
    int active_param = 0;
    REQUIRE(f.FindClosestCallNameInBuffer(CharPos(f, '('), &active_param) ==
            "abcd");
    REQUIRE(active_param == 0);
    REQUIRE(f.FindClosestCallNameInBuffer(CharPos(f, 'e'), &active_param) ==
            "abcd");
    REQUIRE(active_param == 0);
    REQUIRE(f.FindClosestCallNameInBuffer(CharPos(f, 'f'), &active_param) ==
            "abcd");
    REQUIRE(active_param == 0);
    REQUIRE(f.FindClosestCallNameInBuffer(CharPos(f, 'g'), &active_param) ==
            "abcd");
    REQUIRE(active_param == 0);
    REQUIRE(f.FindClosestCallNameInBuffer(CharPos(f, 'g', 1), &active_param) ==
            "efg");
    REQUIRE(active_param == 0);
    REQUIRE(f.FindClosestCallNameInBuffer(CharPos(f, 'g', 2), &active_param) ==
            "efg");
    REQUIRE(active_param == 0);
    REQUIRE(f.FindClosestCallNameInBuffer(CharPos(f, ','), &active_param) ==
            "abcd");
    REQUIRE(active_param == 1);
    REQUIRE(f.FindClosestCallNameInBuffer(CharPos(f, ' '), &active_param) ==
            "abcd");
    REQUIRE(active_param == 1);
  }

  TEST_CASE("auto-insert )") {
    WorkingFile f("foo.cc", "abc()");
    int active_param = 0;
    REQUIRE(f.FindClosestCallNameInBuffer(CharPos(f, ')'), &active_param) ==
            "abc");
    REQUIRE(active_param == 0);
  }

  TEST_CASE("existing completion") {
    WorkingFile f("foo.cc", "zzz.asdf");
    bool is_global_completion;
    std::string existing_completion;

    f.FindStableCompletionSource(CharPos(f, '.'), &is_global_completion,
                                 &existing_completion);
    REQUIRE(existing_completion == "zzz");
    f.FindStableCompletionSource(CharPos(f, 'a', 1), &is_global_completion,
                                 &existing_completion);
    REQUIRE(existing_completion == "a");
    f.FindStableCompletionSource(CharPos(f, 's', 1), &is_global_completion,
                                 &existing_completion);
    REQUIRE(existing_completion == "as");
    f.FindStableCompletionSource(CharPos(f, 'd', 1), &is_global_completion,
                                 &existing_completion);
    REQUIRE(existing_completion == "asd");
    f.FindStableCompletionSource(CharPos(f, 'f', 1), &is_global_completion,
                                 &existing_completion);
    REQUIRE(existing_completion == "asdf");
  }

  TEST_CASE("existing completion underscore") {
    WorkingFile f("foo.cc", "ABC_DEF");
    bool is_global_completion;
    std::string existing_completion;

    f.FindStableCompletionSource(CharPos(f, 'C'), &is_global_completion,
                                 &existing_completion);
    REQUIRE(existing_completion == "AB");
    f.FindStableCompletionSource(CharPos(f, '_'), &is_global_completion,
                                 &existing_completion);
    REQUIRE(existing_completion == "ABC");
    f.FindStableCompletionSource(CharPos(f, 'D'), &is_global_completion,
                                 &existing_completion);
    REQUIRE(existing_completion == "ABC_");
  }
}
