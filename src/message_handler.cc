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

#include "message_handler.h"

#include "log.hh"
#include "match.h"
#include "pipeline.hh"
#include "project.h"
#include "query_utils.h"
using namespace ccls;

using namespace clang;

#include <algorithm>

MAKE_HASHABLE(SymbolIdx, t.usr, t.kind);

namespace {

struct Out_CclsSetSkippedRanges
    : public lsOutMessage<Out_CclsSetSkippedRanges> {
  struct Params {
    lsDocumentUri uri;
    std::vector<lsRange> skippedRanges;
  };
  std::string method = "$ccls/setSkippedRanges";
  Params params;
};
MAKE_REFLECT_STRUCT(Out_CclsSetSkippedRanges::Params, uri, skippedRanges);
MAKE_REFLECT_STRUCT(Out_CclsSetSkippedRanges, jsonrpc, method, params);

struct ScanLineEvent {
  lsPosition pos;
  lsPosition end_pos; // Second key when there is a tie for insertion events.
  int id;
  Out_CclsPublishSemanticHighlighting::Symbol *symbol;
  bool operator<(const ScanLineEvent &other) const {
    // See the comments below when insertion/deletion events are inserted.
    if (!(pos == other.pos))
      return pos < other.pos;
    if (!(other.end_pos == end_pos))
      return other.end_pos < end_pos;
    // This comparison essentially order Macro after non-Macro,
    // So that macros will not be rendered as Var/Type/...
    return symbol->kind < other.symbol->kind;
  }
};
} // namespace

MessageHandler::MessageHandler() {
  // Dynamically allocate |message_handlers|, otherwise there will be static
  // initialization order races.
  if (!message_handlers)
    message_handlers = new std::vector<MessageHandler *>();
  message_handlers->push_back(this);
}

// static
std::vector<MessageHandler *> *MessageHandler::message_handlers = nullptr;

bool FindFileOrFail(DB *db, Project *project, std::optional<lsRequestId> id,
                    const std::string &absolute_path,
                    QueryFile **out_query_file, int *out_file_id) {
  *out_query_file = nullptr;

  auto it = db->name2file_id.find(LowerPathIfInsensitive(absolute_path));
  if (it != db->name2file_id.end()) {
    QueryFile &file = db->files[it->second];
    if (file.def) {
      *out_query_file = &file;
      if (out_file_id)
        *out_file_id = it->second;
      return true;
    }
  }

  if (out_file_id)
    *out_file_id = -1;

  bool indexing;
  {
    std::lock_guard<std::mutex> lock(project->mutex_);
    indexing = project->path_to_entry_index.find(absolute_path) !=
               project->path_to_entry_index.end();
  }
  if (indexing)
    LOG_S(INFO) << "\"" << absolute_path << "\" is being indexed.";
  else
    LOG_S(INFO) << "unable to find file \"" << absolute_path << "\"";

  if (id) {
    Out_Error out;
    out.id = *id;
    if (indexing) {
      out.error.code = lsErrorCodes::ServerNotInitialized;
      out.error.message = absolute_path + " is being indexed.";
    } else {
      out.error.code = lsErrorCodes::InternalError;
      out.error.message = "Unable to find file " + absolute_path;
    }
    pipeline::WriteStdout(kMethodType_Unknown, out);
  }

  return false;
}

void EmitSkippedRanges(WorkingFile *working_file,
                       const std::vector<Range> &skipped_ranges) {
  Out_CclsSetSkippedRanges out;
  out.params.uri = lsDocumentUri::FromPath(working_file->filename);
  for (Range skipped : skipped_ranges) {
    std::optional<lsRange> ls_skipped = GetLsRange(working_file, skipped);
    if (ls_skipped)
      out.params.skippedRanges.push_back(*ls_skipped);
  }
  pipeline::WriteStdout(kMethodType_CclsPublishSkippedRanges, out);
}

void EmitSemanticHighlighting(DB *db, WorkingFile *wfile, QueryFile *file) {
  static GroupMatch match(g_config->highlight.whitelist,
                          g_config->highlight.blacklist);
  assert(file->def);
  if (wfile->buffer_content.size() > g_config->highlight.largeFileSize ||
      !match.IsMatch(file->def->path))
    return;

  // Group symbols together.
  std::unordered_map<SymbolIdx, Out_CclsPublishSemanticHighlighting::Symbol>
      grouped_symbols;
  for (auto &sym_refcnt : file->symbol2refcnt) {
    if (sym_refcnt.second <= 0) continue;
    SymbolRef sym = sym_refcnt.first;
    std::string_view detailed_name;
    lsSymbolKind parent_kind = lsSymbolKind::Unknown;
    lsSymbolKind kind = lsSymbolKind::Unknown;
    uint8_t storage = SC_None;
    int idx;
    // This switch statement also filters out symbols that are not highlighted.
    switch (sym.kind) {
    case SymbolKind::Func: {
      idx = db->func_usr[sym.usr];
      const QueryFunc &func = db->funcs[idx];
      const QueryFunc::Def *def = func.AnyDef();
      if (!def)
        continue; // applies to for loop
      // Don't highlight overloadable operators or implicit lambda ->
      // std::function constructor.
      std::string_view short_name = def->Name(false);
      if (short_name.compare(0, 8, "operator") == 0)
        continue; // applies to for loop
      kind = def->kind;
      storage = def->storage;
      detailed_name = short_name;
      parent_kind = def->parent_kind;

      // Check whether the function name is actually there.
      // If not, do not publish the semantic highlight.
      // E.g. copy-initialization of constructors should not be highlighted
      // but we still want to keep the range for jumping to definition.
      std::string_view concise_name =
          detailed_name.substr(0, detailed_name.find('<'));
      int16_t start_line = sym.range.start.line;
      int16_t start_col = sym.range.start.column;
      if (start_line < 0 || start_line >= wfile->index_lines.size())
        continue;
      std::string_view line = wfile->index_lines[start_line];
      sym.range.end.line = start_line;
      if (!(start_col + concise_name.size() <= line.size() &&
            line.compare(start_col, concise_name.size(), concise_name) == 0))
        continue;
      sym.range.end.column = start_col + concise_name.size();
      break;
    }
    case SymbolKind::Type: {
      idx = db->type_usr[sym.usr];
      const QueryType &type = db->types[idx];
      for (auto &def : type.def) {
        kind = def.kind;
        detailed_name = def.detailed_name;
        if (def.spell) {
          parent_kind = def.parent_kind;
          break;
        }
      }
      break;
    }
    case SymbolKind::Var: {
      idx = db->var_usr[sym.usr];
      const QueryVar &var = db->vars[idx];
      for (auto &def : var.def) {
        kind = def.kind;
        storage = def.storage;
        detailed_name = def.detailed_name;
        if (def.spell) {
          parent_kind = def.parent_kind;
          break;
        }
      }
      break;
    }
    default:
      continue; // applies to for loop
    }

    std::optional<lsRange> loc = GetLsRange(wfile, sym.range);
    if (loc) {
      auto it = grouped_symbols.find(sym);
      if (it != grouped_symbols.end()) {
        it->second.lsRanges.push_back(*loc);
      } else {
        Out_CclsPublishSemanticHighlighting::Symbol symbol;
        symbol.stableId = idx;
        symbol.parentKind = parent_kind;
        symbol.kind = kind;
        symbol.storage = storage;
        symbol.lsRanges.push_back(*loc);
        grouped_symbols[sym] = symbol;
      }
    }
  }

  // Make ranges non-overlapping using a scan line algorithm.
  std::vector<ScanLineEvent> events;
  int id = 0;
  for (auto &entry : grouped_symbols) {
    Out_CclsPublishSemanticHighlighting::Symbol &symbol = entry.second;
    for (auto &loc : symbol.lsRanges) {
      // For ranges sharing the same start point, the one with leftmost end
      // point comes first.
      events.push_back({loc.start, loc.end, id, &symbol});
      // For ranges sharing the same end point, their relative order does not
      // matter, therefore we arbitrarily assign loc.end to them. We use
      // negative id to indicate a deletion event.
      events.push_back({loc.end, loc.end, ~id, &symbol});
      id++;
    }
    symbol.lsRanges.clear();
  }
  std::sort(events.begin(), events.end());

  std::vector<uint8_t> deleted(id, 0);
  int top = 0;
  for (size_t i = 0; i < events.size(); i++) {
    while (top && deleted[events[top - 1].id])
      top--;
    // Order [a, b0) after [a, b1) if b0 < b1. The range comes later overrides
    // the ealier. The order of [a0, b) [a1, b) does not matter.
    // The order of [a, b) [b, c) does not as long as we do not emit empty
    // ranges.
    // Attribute range [events[i-1].pos, events[i].pos) to events[top-1].symbol
    // .
    if (top && !(events[i - 1].pos == events[i].pos))
      events[top - 1].symbol->lsRanges.push_back(
          {events[i - 1].pos, events[i].pos});
    if (events[i].id >= 0)
      events[top++] = events[i];
    else
      deleted[~events[i].id] = 1;
  }

  Out_CclsPublishSemanticHighlighting out;
  out.params.uri = lsDocumentUri::FromPath(wfile->filename);
  // Transform lsRange into pair<int, int> (offset pairs)
  if (!g_config->highlight.lsRanges) {
    std::vector<
        std::pair<lsRange, Out_CclsPublishSemanticHighlighting::Symbol *>>
        scratch;
    for (auto &entry : grouped_symbols) {
      for (auto &range : entry.second.lsRanges)
        scratch.emplace_back(range, &entry.second);
      entry.second.lsRanges.clear();
    }
    std::sort(scratch.begin(), scratch.end(),
              [](auto &l, auto &r) { return l.first.start < r.first.start; });
    const auto &buf = wfile->buffer_content;
    int l = 0, c = 0, i = 0, p = 0;
    auto mov = [&](int line, int col) {
      if (l < line)
        c = 0;
      for (; l < line && i < buf.size(); i++) {
        if (buf[i] == '\n')
          l++;
        if (uint8_t(buf[i]) < 128 || 192 <= uint8_t(buf[i]))
          p++;
      }
      if (l < line)
        return true;
      for (; c < col && i < buf.size() && buf[i] != '\n'; c++)
        if (p++, uint8_t(buf[i++]) >= 128)
          // Skip 0b10xxxxxx
          while (i < buf.size() && uint8_t(buf[i]) >= 128 &&
                 uint8_t(buf[i]) < 192)
            i++;
      return c < col;
    };
    for (auto &entry : scratch) {
      lsRange &r = entry.first;
      if (mov(r.start.line, r.start.character))
        continue;
      int beg = p;
      if (mov(r.end.line, r.end.character))
        continue;
      entry.second->ranges.emplace_back(beg, p);
    }
  }

  for (auto &entry : grouped_symbols)
    if (entry.second.ranges.size() || entry.second.lsRanges.size())
      out.params.symbols.push_back(std::move(entry.second));
  pipeline::WriteStdout(kMethodType_CclsPublishSemanticHighlighting, out);
}
