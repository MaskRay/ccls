#pragma once

#include "language_server_api.h"
#include "utils.h"

#include <clang-c/Index.h>

#include <string>

struct WorkingFile {
  struct Change {
    //
    // A change at disk_line=2, delta=3 means that we are prepending 3 lines
    // before disk_line=2. This means disk_line=2 is shifted to buffer_line=5.
    //
    //  buffer_line=1   disk_line=1   type=disk
    //  buffer_line=2   disk_line=2   type=virtual
    //  buffer_line=3   disk_line=2   type=virtual
    //  buffer_line=4   disk_line=2   type=virtual
    //  buffer_line=5   disk_line=2   type=disk
    //  buffer_line=6   disk_line=3   type=disk
    //
    //
    // A change at disk_line=2, delta=-3 means that we are removing 3 lines
    // starting at disk_line=2. This means disk_line=2,3,4 are removed.
    //
    //  buffer_line=1   disk_line=1   type=disk
    //  buffer_line=2   disk_line=5   type=disk
    //  buffer_line=3   disk_line=6   type=disk
    //
    // There is no meaningful way to map disk_line=2,3,4 to buffer_line; at the
    // moment they are shifted to buffer_line=2. Ideally calling code checks
    // IsDeletedLine and does not emit the reference if that's the case.
    //

    int disk_line;
    int delta;

    Change(int disk_line, int delta);
  };

  std::string filename;
  std::string content;
  std::vector<Change> changes;

  WorkingFile(const std::string& filename, const std::string& content);

  CXUnsavedFile AsUnsavedFile() const;

  // TODO: Add IsDeletedLine(int disk_line) and avoid showing code lens info for that line.

  // Returns the disk line prior to buffer_line.
  //
  // - If there is a change at the start of the document and a buffer_line in that change
  //   is requested, 1 is returned.
  // - If buffer_line is contained within a change (ie, IsBufferLineOnlyInBuffer is true),
  //   then the line immediately preceding that change is returned.
  //
  // Otherwise, the actual disk line is returned.
  int GetDiskLineFromBufferLine(int buffer_line) const;
  
  int GetBufferLineFromDiskLine(int disk_line) const;

  bool IsDeletedDiskLine(int disk_line) const;
  
  bool IsBufferLineOnlyInBuffer(int buffer_line) const;
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