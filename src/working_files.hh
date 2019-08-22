// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "lsp.hh"
#include "utils.hh"

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

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
  void setIndexContent(const std::string &index_content);
  // This should be called whenever |buffer_content| has changed.
  void onBufferContentUpdated();

  // Finds the buffer line number which maps to index line number |line|.
  // Also resolves |column| if not NULL.
  // When resolving a range, use is_end = false for begin() and is_end =
  // true for end() to get a better alignment of |column|.
  std::optional<int> getBufferPosFromIndexPos(int line, int *column,
                                              bool is_end);
  // Finds the index line number which maps to buffer line number |line|.
  // Also resolves |column| if not NULL.
  std::optional<int> getIndexPosFromBufferPos(int line, int *column,
                                              bool is_end);
  // Returns the stable completion position (it jumps back until there is a
  // non-alphanumeric character).
  Position getCompletionPosition(Position pos, std::string *filter,
                                 Position *replace_end_pos) const;

private:
  // Compute index_to_buffer and buffer_to_index.
  void computeLineMapping();
};

struct WorkingFiles {
  WorkingFile *getFile(const std::string &path);
  WorkingFile *getFileUnlocked(const std::string &path);
  std::string getContent(const std::string &path);

  template <typename Fn> void withLock(Fn &&fn) {
    std::lock_guard lock(mutex);
    fn();
  }

  WorkingFile *onOpen(const TextDocumentItem &open);
  void onChange(const TextDocumentDidChangeParam &change);
  void onClose(const std::string &close);

  std::mutex mutex;
  std::unordered_map<std::string, std::unique_ptr<WorkingFile>> files;
};

int getOffsetForPosition(Position pos, std::string_view content);

std::string_view lexIdentifierAroundPos(Position position,
                                        std::string_view content);
} // namespace ccls
