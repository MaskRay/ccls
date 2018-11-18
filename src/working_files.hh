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

#pragma once

#include "lsp.hh"
#include "utils.hh"

#include <mutex>
#include <optional>
#include <string>

namespace ccls {
struct WorkingFile {
  int version = 0;
  std::string filename;

  std::string buffer_content;
  // Note: This assumes 0-based lines (1-based lines are normally assumed).
  std::vector<std::string> index_lines;
  // Note: This assumes 0-based lines (1-based lines are normally assumed).
  std::vector<std::string> buffer_lines;
  // Mappings between index line number and buffer line number.
  // Empty indicates either buffer or index has been changed and re-computation
  // is required.
  // For index_to_buffer[i] == j, if j >= 0, we are confident that index line
  // i maps to buffer line j; if j == -1, FindMatchingLine will use the nearest
  // confident lines to resolve its line number.
  std::vector<int> index_to_buffer;
  std::vector<int> buffer_to_index;
  // A set of diagnostics that have been reported for this file.
  // NOTE: _ is appended because it must be accessed under the WorkingFiles
  // lock!
  std::vector<Diagnostic> diagnostics_;

  WorkingFile(const std::string &filename, const std::string &buffer_content);

  // This should be called when the indexed content has changed.
  void SetIndexContent(const std::string &index_content);
  // This should be called whenever |buffer_content| has changed.
  void OnBufferContentUpdated();

  // Finds the buffer line number which maps to index line number |line|.
  // Also resolves |column| if not NULL.
  // When resolving a range, use is_end = false for begin() and is_end =
  // true for end() to get a better alignment of |column|.
  std::optional<int> GetBufferPosFromIndexPos(int line, int *column,
                                              bool is_end);
  // Finds the index line number which maps to buffer line number |line|.
  // Also resolves |column| if not NULL.
  std::optional<int> GetIndexPosFromBufferPos(int line, int *column,
                                              bool is_end);
  // Returns a relatively stable completion position (it jumps back until there
  // is a non-alphanumeric character).
  //
  // The out param |is_global_completion| is set to true if this looks like a
  // global completion.
  // The out param |existing_completion| is set to any existing completion
  // content the user has entered.
  Position FindStableCompletionSource(Position position,
                                        std::string *existing_completion,
                                        Position *replace_end_pos) const;

private:
  // Compute index_to_buffer and buffer_to_index.
  void ComputeLineMapping();
};

struct WorkingFiles {
  struct Snapshot {
    struct File {
      std::string filename;
      std::string content;
    };

    std::vector<File> files;
  };

  //
  // :: IMPORTANT :: All methods in this class are guarded by a single lock.
  //

  // Find the file with the given filename.
  WorkingFile *GetFileByFilename(const std::string &filename);
  WorkingFile *GetFileByFilenameNoLock(const std::string &filename);
  std::string GetContent(const std::string &filename);

  // Run |action| under the lock.
  template <typename Fn> void DoAction(Fn &&fn) {
    std::lock_guard<std::mutex> lock(files_mutex);
    fn();
  }

  WorkingFile *OnOpen(const TextDocumentItem &open);
  void OnChange(const TextDocumentDidChangeParam &change);
  void OnClose(const TextDocumentIdentifier &close);

  // Use unique_ptrs so we can handout WorkingFile ptrs and not have them
  // invalidated if we resize files.
  std::vector<std::unique_ptr<WorkingFile>> files;
  std::mutex files_mutex; // Protects |files|.
};

int GetOffsetForPosition(Position pos, std::string_view content);

std::string_view LexIdentifierAroundPos(Position position,
                                        std::string_view content);
} // namespace ccls
