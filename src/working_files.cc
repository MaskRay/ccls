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

#include "working_files.hh"

#include "log.hh"
#include "position.hh"

#include <clang/Basic/CharInfo.h>

#include <algorithm>
#include <chrono>
#include <climits>
#include <numeric>
#include <sstream>
namespace chrono = std::chrono;

using namespace clang;
using namespace llvm;

namespace ccls {
namespace {

// When finding a best match of buffer line and index line, limit the max edit
// distance.
constexpr int kMaxDiff = 20;
// Don't align index line to buffer line if one of the lengths is larger than
// |kMaxColumnAlignSize|.
constexpr int kMaxColumnAlignSize = 200;

Position GetPositionForOffset(const std::string &content, int offset) {
  if (offset >= content.size())
    offset = (int)content.size() - 1;

  int line = 0, col = 0;
  int i = 0;
  for (; i < offset; i++) {
    if (content[i] == '\n')
      line++, col = 0;
    else
      col++;
  }
  return {line, col};
}

std::vector<std::string> ToLines(const std::string &content) {
  std::vector<std::string> result;
  std::istringstream lines(content);
  std::string line;
  while (getline(lines, line))
    result.push_back(line);
  return result;
}

// Computes the edit distance of strings [a,a+la) and [b,b+lb) with Eugene W.
// Myers' O(ND) diff algorithm.
// Costs: insertion=1, deletion=1, no substitution.
// If the distance is larger than threshold, returns threshould + 1.
int MyersDiff(const char *a, int la, const char *b, int lb, int threshold) {
  assert(threshold <= kMaxDiff);
  static int v_static[2 * kMaxColumnAlignSize + 2];
  const char *ea = a + la, *eb = b + lb;
  // Strip prefix
  for (; a < ea && b < eb && *a == *b; a++, b++) {
  }
  // Strip suffix
  for (; a < ea && b < eb && ea[-1] == eb[-1]; ea--, eb--) {
  }
  la = int(ea - a);
  lb = int(eb - b);
  // If the sum of lengths exceeds what we can handle, return a lower bound.
  if (la + lb > 2 * kMaxColumnAlignSize)
    return std::min(abs(la - lb), threshold + 1);

  int *v = v_static + lb;
  v[1] = 0;
  for (int di = 0; di <= threshold; di++) {
    int low = -di + 2 * std::max(0, di - lb),
        high = di - 2 * std::max(0, di - la);
    for (int i = low; i <= high; i += 2) {
      int x = i == -di || (i != di && v[i - 1] < v[i + 1]) ? v[i + 1]
                                                           : v[i - 1] + 1,
          y = x - i;
      while (x < la && y < lb && a[x] == b[y])
        x++, y++;
      v[i] = x;
      if (x == la && y == lb)
        return di;
    }
  }
  return threshold + 1;
}

int MyersDiff(const std::string &a, const std::string &b, int threshold) {
  return MyersDiff(a.data(), a.size(), b.data(), b.size(), threshold);
}

// Computes edit distance with O(N*M) Needleman-Wunsch algorithm
// and returns a distance vector where d[i] = cost of aligning a to b[0,i).
//
// Myers' diff algorithm is used to find best matching line while this one is
// used to align a single column because Myers' needs some twiddling to return
// distance vector.
std::vector<int> EditDistanceVector(std::string a, std::string b) {
  std::vector<int> d(b.size() + 1);
  std::iota(d.begin(), d.end(), 0);
  for (int i = 0; i < (int)a.size(); i++) {
    int ul = d[0];
    d[0] = i + 1;
    for (int j = 0; j < (int)b.size(); j++) {
      int t = d[j + 1];
      d[j + 1] = a[i] == b[j] ? ul : std::min(d[j], d[j + 1]) + 1;
      ul = t;
    }
  }
  return d;
}

// Find matching position of |a[column]| in |b|.
// This is actually a single step of Hirschberg's sequence alignment algorithm.
int AlignColumn(const std::string &a, int column, std::string b, bool is_end) {
  int head = 0, tail = 0;
  while (head < (int)a.size() && head < (int)b.size() && a[head] == b[head])
    head++;
  while (tail < (int)a.size() && tail < (int)b.size() &&
         a[a.size() - 1 - tail] == b[b.size() - 1 - tail])
    tail++;
  if (column < head)
    return column;
  if ((int)a.size() - tail < column)
    return column + b.size() - a.size();
  if (std::max(a.size(), b.size()) - head - tail >= kMaxColumnAlignSize)
    return std::min(column, (int)b.size());

  // b[head, b.size() - tail)
  b = b.substr(head, b.size() - tail - head);

  // left[i] = cost of aligning a[head, column) to b[head, head + i)
  std::vector<int> left = EditDistanceVector(a.substr(head, column - head), b);

  // right[i] = cost of aligning a[column, a.size() - tail) to b[head + i,
  // b.size() - tail)
  std::string a_rev = a.substr(column, a.size() - tail - column);
  std::reverse(a_rev.begin(), a_rev.end());
  std::reverse(b.begin(), b.end());
  std::vector<int> right = EditDistanceVector(a_rev, b);
  std::reverse(right.begin(), right.end());

  int best = 0, best_cost = INT_MAX;
  for (size_t i = 0; i < left.size(); i++) {
    int cost = left[i] + right[i];
    if (is_end ? cost < best_cost : cost <= best_cost) {
      best_cost = cost;
      best = i;
    }
  }
  return head + best;
}

// Find matching buffer line of index_lines[line].
// By symmetry, this can also be used to find matching index line of a buffer
// line.
std::optional<int>
FindMatchingLine(const std::vector<std::string> &index_lines,
                 const std::vector<int> &index_to_buffer, int line, int *column,
                 const std::vector<std::string> &buffer_lines, bool is_end) {
  // If this is a confident mapping, returns.
  if (index_to_buffer[line] >= 0) {
    int ret = index_to_buffer[line];
    if (column)
      *column =
          AlignColumn(index_lines[line], *column, buffer_lines[ret], is_end);
    return ret;
  }

  // Find the nearest two confident lines above and below.
  int up = line, down = line;
  while (--up >= 0 && index_to_buffer[up] < 0) {
  }
  while (++down < int(index_to_buffer.size()) && index_to_buffer[down] < 0) {
  }
  up = up < 0 ? 0 : index_to_buffer[up];
  down = down >= int(index_to_buffer.size()) ? int(buffer_lines.size()) - 1
                                             : index_to_buffer[down];
  if (up > down)
    return std::nullopt;

  // Search for lines [up,down] and use Myers's diff algorithm to find the best
  // match (least edit distance).
  int best = up, best_dist = kMaxDiff + 1;
  const std::string &needle = index_lines[line];
  for (int i = up; i <= down; i++) {
    int dist = MyersDiff(needle, buffer_lines[i], kMaxDiff);
    if (dist < best_dist) {
      best_dist = dist;
      best = i;
    }
  }
  if (column)
    *column =
        AlignColumn(index_lines[line], *column, buffer_lines[best], is_end);
  return best;
}

} // namespace

WorkingFile::WorkingFile(const std::string &filename,
                         const std::string &buffer_content)
    : filename(filename), buffer_content(buffer_content) {
  OnBufferContentUpdated();

  // SetIndexContent gets called when the file is opened.
}

void WorkingFile::SetIndexContent(const std::string &index_content) {
  index_lines = ToLines(index_content);

  index_to_buffer.clear();
  buffer_to_index.clear();
}

void WorkingFile::OnBufferContentUpdated() {
  buffer_lines = ToLines(buffer_content);

  index_to_buffer.clear();
  buffer_to_index.clear();
}

// Variant of Paul Heckel's diff algorithm to compute |index_to_buffer| and
// |buffer_to_index|.
// The core idea is that if a line is unique in both index and buffer,
// we are confident that the line appeared in index maps to the one appeared in
// buffer. And then using them as start points to extend upwards and downwards
// to align other identical lines (but not unique).
void WorkingFile::ComputeLineMapping() {
  std::unordered_map<uint64_t, int> hash_to_unique;
  std::vector<uint64_t> index_hashes(index_lines.size());
  std::vector<uint64_t> buffer_hashes(buffer_lines.size());
  index_to_buffer.resize(index_lines.size());
  buffer_to_index.resize(buffer_lines.size());
  hash_to_unique.reserve(
      std::max(index_to_buffer.size(), buffer_to_index.size()));

  // For index line i, set index_to_buffer[i] to -1 if line i is duplicated.
  int i = 0;
  for (StringRef line : index_lines) {
    uint64_t h = HashUsr(line);
    auto it = hash_to_unique.find(h);
    if (it == hash_to_unique.end()) {
      hash_to_unique[h] = i;
      index_to_buffer[i] = i;
    } else {
      if (it->second >= 0)
        index_to_buffer[it->second] = -1;
      index_to_buffer[i] = it->second = -1;
    }
    index_hashes[i++] = h;
  }

  // For buffer line i, set buffer_to_index[i] to -1 if line i is duplicated.
  i = 0;
  hash_to_unique.clear();
  for (StringRef line : buffer_lines) {
    uint64_t h = HashUsr(line);
    auto it = hash_to_unique.find(h);
    if (it == hash_to_unique.end()) {
      hash_to_unique[h] = i;
      buffer_to_index[i] = i;
    } else {
      if (it->second >= 0)
        buffer_to_index[it->second] = -1;
      buffer_to_index[i] = it->second = -1;
    }
    buffer_hashes[i++] = h;
  }

  // If index line i is the identical to buffer line j, and they are both
  // unique, align them by pointing from_index[i] to j.
  i = 0;
  for (auto h : index_hashes) {
    if (index_to_buffer[i] >= 0) {
      auto it = hash_to_unique.find(h);
      if (it != hash_to_unique.end() && it->second >= 0 &&
          buffer_to_index[it->second] >= 0)
        index_to_buffer[i] = it->second;
      else
        index_to_buffer[i] = -1;
    }
    i++;
  }

  // Starting at unique lines, extend upwards and downwards.
  for (i = 0; i < (int)index_hashes.size() - 1; i++) {
    int j = index_to_buffer[i];
    if (0 <= j && j + 1 < buffer_hashes.size() &&
        index_hashes[i + 1] == buffer_hashes[j + 1])
      index_to_buffer[i + 1] = j + 1;
  }
  for (i = (int)index_hashes.size(); --i > 0;) {
    int j = index_to_buffer[i];
    if (0 < j && index_hashes[i - 1] == buffer_hashes[j - 1])
      index_to_buffer[i - 1] = j - 1;
  }

  // |buffer_to_index| is a inverse mapping of |index_to_buffer|.
  std::fill(buffer_to_index.begin(), buffer_to_index.end(), -1);
  for (i = 0; i < (int)index_hashes.size(); i++)
    if (index_to_buffer[i] >= 0)
      buffer_to_index[index_to_buffer[i]] = i;
}

std::optional<int> WorkingFile::GetBufferPosFromIndexPos(int line, int *column,
                                                         bool is_end) {
  if (line == (int)index_lines.size() && !*column)
    return buffer_content.size();
  if (line < 0 || line >= (int)index_lines.size()) {
    LOG_S(WARNING) << "bad index_line (got " << line << ", expected [0, "
                   << index_lines.size() << ")) in " << filename;
    return std::nullopt;
  }

  if (index_to_buffer.empty())
    ComputeLineMapping();
  return FindMatchingLine(index_lines, index_to_buffer, line, column,
                          buffer_lines, is_end);
}

std::optional<int> WorkingFile::GetIndexPosFromBufferPos(int line, int *column,
                                                         bool is_end) {
  if (line < 0 || line >= (int)buffer_lines.size())
    return std::nullopt;

  if (buffer_to_index.empty())
    ComputeLineMapping();
  return FindMatchingLine(buffer_lines, buffer_to_index, line, column,
                          index_lines, is_end);
}

Position WorkingFile::GetCompletionPosition(Position pos, std::string *filter,
                                            Position *replace_end_pos) const {
  int start = GetOffsetForPosition(pos, buffer_content);
  int i = start;
  while (i > 0 && isIdentifierBody(buffer_content[i - 1]))
    --i;

  *replace_end_pos = pos;
  for (int i = start;
       i < buffer_content.size() && isIdentifierBody(buffer_content[i]); i++)
    replace_end_pos->character++;

  *filter = buffer_content.substr(i, start - i);
  return GetPositionForOffset(buffer_content, i);
}

WorkingFile *WorkingFiles::GetFile(const std::string &path) {
  std::lock_guard lock(mutex);
  return GetFileUnlocked(path);
}

WorkingFile *WorkingFiles::GetFileUnlocked(const std::string &path) {
  auto it = files.find(path);
  return it != files.end() ? it->second.get() : nullptr;
}

std::string WorkingFiles::GetContent(const std::string &path) {
  std::lock_guard lock(mutex);
  auto it = files.find(path);
  return it != files.end() ? it->second->buffer_content : "";
}

WorkingFile *WorkingFiles::OnOpen(const TextDocumentItem &open) {
  std::lock_guard lock(mutex);

  std::string path = open.uri.GetPath();
  std::string content = open.text;

  auto &wf = files[path];
  if (wf) {
    wf->version = open.version;
    wf->buffer_content = content;
    wf->OnBufferContentUpdated();
  } else {
    wf = std::make_unique<WorkingFile>(path, content);
  }
  return wf.get();
}

void WorkingFiles::OnChange(const TextDocumentDidChangeParam &change) {
  std::lock_guard lock(mutex);

  std::string path = change.textDocument.uri.GetPath();
  WorkingFile *file = GetFileUnlocked(path);
  if (!file) {
    LOG_S(WARNING) << "Could not change " << path << " because it was not open";
    return;
  }

  file->timestamp = chrono::duration_cast<chrono::seconds>(
                        chrono::high_resolution_clock::now().time_since_epoch())
                        .count();

  // version: number | null
  if (change.textDocument.version)
    file->version = *change.textDocument.version;

  for (const TextDocumentContentChangeEvent &diff : change.contentChanges) {
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
      int end_offset =
          GetOffsetForPosition(diff.range->end, file->buffer_content);
      file->buffer_content.replace(file->buffer_content.begin() + start_offset,
                                   file->buffer_content.begin() + end_offset,
                                   diff.text);
      file->OnBufferContentUpdated();
    }
  }
}

void WorkingFiles::OnClose(const std::string &path) {
  std::lock_guard lock(mutex);
  files.erase(path);
}

// VSCode (UTF-16) disagrees with Emacs lsp-mode (UTF-8) on how to represent
// text documents.
// We use a UTF-8 iterator to approximate UTF-16 in the specification (weird).
// This is good enough and fails only for UTF-16 surrogate pairs.
int GetOffsetForPosition(Position pos, std::string_view content) {
  size_t i = 0;
  for (; pos.line > 0 && i < content.size(); i++)
    if (content[i] == '\n')
      pos.line--;
  for (; pos.character > 0 && i < content.size() && content[i] != '\n';
       pos.character--)
    if (uint8_t(content[i++]) >= 128) {
      // Skip 0b10xxxxxx
      while (i < content.size() && uint8_t(content[i]) >= 128 &&
             uint8_t(content[i]) < 192)
        i++;
    }
  return int(i);
}

std::string_view LexIdentifierAroundPos(Position position,
                                        std::string_view content) {
  int start = GetOffsetForPosition(position, content), end = start + 1;
  char c;

  // We search for :: before the cursor but not after to get the qualifier.
  for (; start > 0; start--) {
    c = content[start - 1];
    if (c == ':' && start > 1 && content[start - 2] == ':')
      start--;
    else if (!isIdentifierBody(c))
      break;
  }
  for (; end < content.size() && isIdentifierBody(content[end]); end++)
    ;

  return content.substr(start, end - start);
}
}
