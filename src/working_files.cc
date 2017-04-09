#include "working_files.h"

#include <doctest/doctest.h>

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

int LineCount(std::string::const_iterator start, const std::string::const_iterator& end) {
  int count = 0;
  while (start != end) {
    if (*start == '\n')
      ++count;
    ++start;
  }
  return count;
}

bool IsNextTokenNewline(const std::string& str, int start) {
  while (start < str.size()) {
    // Peek a token ahead for \r\n. We cannot check a token back because
    // isalpha('\r') returns false.
    if ((start + 1 < str.size()) && str[start] == '\r' && str[start + 1] == '\n')
      return true;

    if (str[start] == '\n')
      return true;

    if (!isalpha(str[start]))
      return false;

    ++start;
  }

  return true;
}

}  // namespace

WorkingFile::Change::Change(int disk_line, int delta) : disk_line(disk_line), delta(delta) {}

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

int WorkingFile::GetDiskLineFromBufferLine(int buffer_line) const {
  int running_delta = 0;

  // TODO: The disk line could come *after* the buffer line, ie, we insert into the start of the document.
  //  change disk_line=1, delta=4         // insert 4 lines before line 1
  //  buffer_line=5 maps to disk_line=1   
  //

  for (const auto& change : changes) {
    int buffer_start = running_delta + change.disk_line;
    int buffer_end = running_delta + change.disk_line + change.delta;

    if (buffer_line < buffer_start)
      break;

    if (buffer_line >= buffer_start && buffer_line < buffer_end) {
      //if (buffer_line == change.disk_line || change.disk_line == 1) return change.disk_line;
      return change.disk_line;
    }

    running_delta += change.delta;
  }

  return buffer_line - running_delta;
}

int WorkingFile::GetBufferLineFromDiskLine(int disk_line) const {
  int buffer_line = disk_line;

  for (const auto& change : changes) {
    if (disk_line >= change.disk_line) {
      assert(change.delta != 0);

      if (change.delta > 0)
        buffer_line += change.delta;
      else {
        if (disk_line < (change.disk_line + (-change.delta)))
          buffer_line -= disk_line - change.disk_line + 1;
        else
          buffer_line += change.delta;
      }
    }
  }

  //std::cerr << "disk_line " << disk_line << " => buffer_line " << buffer_line << std::endl;// << " (line_deltas.size() = " << line_deltas.size() << ")" << std::endl;
  return buffer_line;
}

bool WorkingFile::IsDeletedDiskLine(int disk_line) const {
  for (const auto& change : changes) {
    if (change.delta < 0 &&
        disk_line >= change.disk_line && disk_line < (change.disk_line + (-change.delta))) {
      return true;
    }
  }

  return false;
}

bool WorkingFile::IsBufferLineOnlyInBuffer(int buffer_line) const {
  //
  //   5: 2
  //   7: 3
  //
  //   1 -> 1
  //   2 -> 2
  //   3 -> 3
  //   4 -> 4
  //   5 -> 7
  //   6 -> 8
  //   7 -> 12
  //   8 -> 13
  //
  // lines 5,6 are virtual
  // lines 9,10,11 are virtual
  //
  //

  int running_delta = 0;
  for (const auto& change : changes) {
    int buffer_start = change.disk_line + running_delta;
    int buffer_end = change.disk_line + running_delta + change.delta;

    if (buffer_line < buffer_start)
      break;

    if (buffer_line >= buffer_start && buffer_line < buffer_end)
      return true;

    running_delta += change.delta;
  }

  return false;
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
    //std::cerr << "Applying rangeLength=" << diff.rangeLength;

    // If range or rangeLength are emitted we replace everything, per the spec.
    if (diff.rangeLength == -1) {
      file->content = diff.text;
    }
    else {
      int start_offset = GetOffsetForPosition(diff.range.start, file->content);
      int previous_line_count = diff.range.end.line - diff.range.start.line;
      int replace_line_count = LineCount(diff.text.begin(), diff.text.end());

      int line_delta = replace_line_count - previous_line_count;
      if (line_delta != 0) {
        // language server stores line counts starting at 0, we start at 1
        int buffer_line = diff.range.start.line + 1;

        // If we're adding a newline but not actually shifting any of the
        // contents of this line, we should mark the text prepended on the next
        // line.
        if (IsNextTokenNewline(file->content, start_offset))
          ++buffer_line;

        int disk_line = file->GetDiskLineFromBufferLine(buffer_line);

        bool found = false;
        for (int i = 0; i < file->changes.size(); ++i) {
          auto& change = file->changes[i];
          if (change.disk_line == disk_line) {
            change.delta += line_delta;
            found = true;
          }

          if (change.delta == 0)
            file->changes.erase(file->changes.begin() + i);
        }
        if (!found)
          file->changes.push_back(WorkingFile::Change(disk_line, line_delta));

        //std::cerr << "(changes.size()=" << file->changes.size() << ") Inserted delta=" << line_delta << " at disk_line=" << disk_line << " with buffer_line=" << buffer_line << std::endl;
        //for (auto& change : file->changes)
        //  std::cerr << "  disk_line=" << change.disk_line << " delta=" << change.delta << std::endl;

        std::sort(file->changes.begin(), file->changes.end(),
          [](const WorkingFile::Change& a, const WorkingFile::Change& b) {
          return a.disk_line < b.disk_line;
        });
      }


      file->content.replace(file->content.begin() + start_offset,
        file->content.begin() + start_offset + diff.rangeLength,
        diff.text);
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


TEST_SUITE("WorkingFiles");

TEST_CASE("buffer-to-disk no changes") {
  WorkingFile f("", "");

  CHECK(f.GetDiskLineFromBufferLine(1) == 1);
  CHECK(f.GetDiskLineFromBufferLine(2) == 2);
  CHECK(f.GetDiskLineFromBufferLine(3) == 3);
}

TEST_CASE("buffer-to-disk start add") {
  WorkingFile f("", "");

  f.changes.push_back(WorkingFile::Change(1, 3));

  CHECK(f.IsBufferLineOnlyInBuffer(1) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(2) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(3) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(4) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(5) == false);

  CHECK(f.GetDiskLineFromBufferLine(1) == 1);
  CHECK(f.GetDiskLineFromBufferLine(2) == 1);
  CHECK(f.GetDiskLineFromBufferLine(3) == 1);
  CHECK(f.GetDiskLineFromBufferLine(4) == 1);
  CHECK(f.GetDiskLineFromBufferLine(5) == 2);
}

TEST_CASE("buffer-to-disk past-start add") {
  WorkingFile f("", "");

  f.changes.push_back(WorkingFile::Change(2, 3));

  CHECK(f.IsBufferLineOnlyInBuffer(1) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(2) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(3) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(4) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(5) == false);

  CHECK(f.GetDiskLineFromBufferLine(1) == 1);
  CHECK(f.GetDiskLineFromBufferLine(2) == 2); // buffer-only
  CHECK(f.GetDiskLineFromBufferLine(3) == 2); // buffer-only
  CHECK(f.GetDiskLineFromBufferLine(4) == 2); // buffer-only
  CHECK(f.GetDiskLineFromBufferLine(5) == 2);
  CHECK(f.GetDiskLineFromBufferLine(6) == 3);
}

TEST_CASE("buffer-to-disk start remove") {
  WorkingFile f("", "");

  f.changes.push_back(WorkingFile::Change(1, -3));

  CHECK(f.IsBufferLineOnlyInBuffer(1) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(2) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(3) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(4) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(5) == false);

  CHECK(f.GetDiskLineFromBufferLine(1) == 4);
  CHECK(f.GetDiskLineFromBufferLine(2) == 5);
  CHECK(f.GetDiskLineFromBufferLine(3) == 6);
  CHECK(f.GetDiskLineFromBufferLine(4) == 7);
  CHECK(f.GetDiskLineFromBufferLine(5) == 8);
}

TEST_CASE("buffer-to-disk past-start remove") {
  WorkingFile f("", "");

  f.changes.push_back(WorkingFile::Change(2, -3));

  CHECK(f.IsBufferLineOnlyInBuffer(1) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(2) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(3) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(4) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(5) == false);

  CHECK(f.GetDiskLineFromBufferLine(1) == 1);
  CHECK(f.GetDiskLineFromBufferLine(2) == 5);
  CHECK(f.GetDiskLineFromBufferLine(3) == 6);
  CHECK(f.GetDiskLineFromBufferLine(4) == 7);
  CHECK(f.GetDiskLineFromBufferLine(5) == 8);
}

TEST_CASE("buffer 1-1 mapping") {
  WorkingFile f("", "");

  f.changes.push_back(WorkingFile::Change(2, 5));
  f.changes.push_back(WorkingFile::Change(3, -2));
  f.changes.push_back(WorkingFile::Change(4, 3));

  for (int i = 1; i < 20; ++i) {
    // TODO: no good disk line from change starting at line 1

    int disk1_line = f.GetDiskLineFromBufferLine(i);
    int buffer_line = f.GetBufferLineFromDiskLine(disk1_line);
    int disk2_line = f.GetDiskLineFromBufferLine(buffer_line);

    CHECK(disk1_line == disk2_line);
  }
}

TEST_CASE("disk-to-buffer deleted disk lines") {
  WorkingFile f("", "");

  f.changes.push_back(WorkingFile::Change(4, -3));

  CHECK(f.GetBufferLineFromDiskLine(1) == 1);
  CHECK(f.GetBufferLineFromDiskLine(2) == 2);
  CHECK(f.GetBufferLineFromDiskLine(3) == 3);
  CHECK(f.GetBufferLineFromDiskLine(4) == 3);
  CHECK(f.GetBufferLineFromDiskLine(5) == 3);
  CHECK(f.GetBufferLineFromDiskLine(6) == 3);
  CHECK(f.GetBufferLineFromDiskLine(7) == 4);
}


TEST_CASE("disk-to-buffer mapping add remove") {
  WorkingFile f("", "");

  f.changes.push_back(WorkingFile::Change(2, 5));
  f.changes.push_back(WorkingFile::Change(10, -2));

  CHECK(f.GetBufferLineFromDiskLine(1) == 1);
  CHECK(f.GetBufferLineFromDiskLine(2) == 7);
  CHECK(f.GetBufferLineFromDiskLine(3) == 8);

  CHECK(f.GetBufferLineFromDiskLine(8) == 13);
  CHECK(f.GetBufferLineFromDiskLine(9) == 14);
  CHECK(f.GetBufferLineFromDiskLine(10) == 14);
  CHECK(f.GetBufferLineFromDiskLine(11) == 14);
  CHECK(f.GetBufferLineFromDiskLine(12) == 15);
}

TEST_CASE("disk-to-buffer mapping add") {
  WorkingFile f("", "");

  f.changes.push_back(WorkingFile::Change(2, 5));

  CHECK(f.GetDiskLineFromBufferLine(1) == 1);
  CHECK(f.GetDiskLineFromBufferLine(2) == 2);
  CHECK(f.GetDiskLineFromBufferLine(3) == 2);
  CHECK(f.GetDiskLineFromBufferLine(4) == 2);
  CHECK(f.GetDiskLineFromBufferLine(5) == 2);
  CHECK(f.GetDiskLineFromBufferLine(6) == 2);
  CHECK(f.GetDiskLineFromBufferLine(7) == 2);
  CHECK(f.GetDiskLineFromBufferLine(8) == 3);
}

TEST_CASE("disk-to-buffer remove lines") {
  WorkingFile f("", "");

  f.changes.push_back(WorkingFile::Change(2, -5));

  CHECK(f.GetDiskLineFromBufferLine(1) == 1);
  CHECK(f.GetDiskLineFromBufferLine(2) == 7);
  CHECK(f.GetDiskLineFromBufferLine(3) == 8);
  CHECK(f.GetDiskLineFromBufferLine(4) == 9);
}


TEST_CASE("disk-to-buffer overlapping") {
  WorkingFile f("", "");

  f.changes.push_back(WorkingFile::Change(1, 5));
  f.changes.push_back(WorkingFile::Change(2, -2));
  f.changes.push_back(WorkingFile::Change(3, 3));

  CHECK(f.GetBufferLineFromDiskLine(1) == 6);
  CHECK(f.GetBufferLineFromDiskLine(2) == 6);
  CHECK(f.GetBufferLineFromDiskLine(3) == 9);
  CHECK(f.GetBufferLineFromDiskLine(4) == 10);
  CHECK(f.GetBufferLineFromDiskLine(5) == 11);
  CHECK(f.GetBufferLineFromDiskLine(6) == 12);

  CHECK(f.IsBufferLineOnlyInBuffer(1) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(2) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(3) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(4) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(5) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(6) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(7) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(8) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(9) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(10) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(11) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(12) == false);
}

TEST_CASE("is buffer line removing lines") {
  WorkingFile f("", "");

  f.changes.push_back(WorkingFile::Change(2, -2));

  CHECK(f.IsBufferLineOnlyInBuffer(1) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(2) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(3) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(4) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(5) == false);
}

TEST_CASE("is buffer line adding lines") {

  //
  //   5: 2
  //   7: 3
  //
  //   1 -> 1
  //   2 -> 2
  //   3 -> 3
  //   4 -> 4
  //   5 -> 7
  //   6 -> 8
  //   7 -> 12
  //   8 -> 13
  //
  // lines 5,6 are virtual
  // lines 9,10,11 are virtual
  //

  WorkingFile f("", "");

  f.changes.push_back(WorkingFile::Change(5, 2));
  f.changes.push_back(WorkingFile::Change(7, 3));

  CHECK(f.GetBufferLineFromDiskLine(1) == 1);
  CHECK(f.GetBufferLineFromDiskLine(2) == 2);
  CHECK(f.GetBufferLineFromDiskLine(3) == 3);
  CHECK(f.GetBufferLineFromDiskLine(4) == 4);
  CHECK(f.GetBufferLineFromDiskLine(5) == 7);
  CHECK(f.GetBufferLineFromDiskLine(6) == 8);
  CHECK(f.GetBufferLineFromDiskLine(7) == 12);
  CHECK(f.GetBufferLineFromDiskLine(8) == 13);

  CHECK(f.IsBufferLineOnlyInBuffer(1) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(2) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(3) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(4) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(5) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(6) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(7) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(8) == false);
  CHECK(f.IsBufferLineOnlyInBuffer(9) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(10) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(11) == true);
  CHECK(f.IsBufferLineOnlyInBuffer(12) == false);
}

TEST_CASE("sanity") {
  /*
  (changes.size()=1) Inserted delta=1 at disk_line=4 with buffer_line=4
    disk_line=4 delta=1
  (changes.size()=1) Inserted delta=1 at disk_line=4 with buffer_line=5
    disk_line=4 delta=2
  (changes.size()=2) Inserted delta=1 at disk_line=3 with buffer_line=5
    disk_line=4 delta=2
    disk_line=3 delta=1
  */

  WorkingFile f("", "");

  f.changes.push_back(WorkingFile::Change(4, 2));

  CHECK(f.GetDiskLineFromBufferLine(5) == 4);
}


TEST_CASE("deleted_line start") {
  WorkingFile f("", "");

  f.changes.push_back(WorkingFile::Change(1, -3));

  CHECK(f.IsDeletedDiskLine(1) == true);
  CHECK(f.IsDeletedDiskLine(2) == true);
  CHECK(f.IsDeletedDiskLine(3) == true);
  CHECK(f.IsDeletedDiskLine(4) == false);
}

TEST_CASE("deleted_line past-start") {
  WorkingFile f("", "");

  f.changes.push_back(WorkingFile::Change(2, -3));

  CHECK(f.IsDeletedDiskLine(1) == false);
  CHECK(f.IsDeletedDiskLine(2) == true);
  CHECK(f.IsDeletedDiskLine(3) == true);
  CHECK(f.IsDeletedDiskLine(4) == true);
  CHECK(f.IsDeletedDiskLine(5) == false);
}

TEST_SUITE_END();