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
  int64_t timestamp = 0;
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
  std::vector<Diagnostic> diagnostics;

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
  // Returns the stable completion position (it jumps back until there is a
  // non-alphanumeric character).
  Position GetCompletionPosition(Position pos, std::string *filter,
                                 Position *replace_end_pos) const;

private:
  // Compute index_to_buffer and buffer_to_index.
  void ComputeLineMapping();
};

struct WorkingFiles {
  WorkingFile *GetFile(const std::string &path);
  WorkingFile *GetFileUnlocked(const std::string &path);
  std::string GetContent(const std::string &path);

  template <typename Fn> void WithLock(Fn &&fn) {
    std::lock_guard lock(mutex);
    fn();
  }

  WorkingFile *OnOpen(const TextDocumentItem &open);
  void OnChange(const TextDocumentDidChangeParam &change);
  void OnClose(const std::string &close);

  std::mutex mutex;
  std::unordered_map<std::string, std::unique_ptr<WorkingFile>> files;
};

int GetOffsetForPosition(Position pos, std::string_view content);

std::string_view LexIdentifierAroundPos(Position position,
                                        std::string_view content);
} // namespace ccls
