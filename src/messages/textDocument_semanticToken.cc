// Copyright ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "indexer.hh"
#include "log.hh"
#include "message_handler.hh"
#include "pipeline.hh"
#include "sema_manager.hh"

#include <algorithm>
#include <clang/Sema/Sema.h>
#include <stdexcept>
#include <string>

MAKE_HASHABLE(ccls::SymbolIdx, t.usr, t.kind);

namespace ccls {
REFLECT_STRUCT(QueryFile::SemanticTokens, data);
REFLECT_STRUCT(QueryFile::SemanticTokensWithId, tokens, id);

using namespace clang;

namespace {

struct CclsSemanticHighlightSymbol {
  int id = 0;
  SymbolKind parentKind;
  SymbolKind kind;
  uint8_t storage;
  std::vector<std::pair<lsRange, Role>> lsRangeAndRoles;
};

struct ScanLineEvent {
  Position pos;
  Position end_pos; // Second key when there is a tie for insertion events.
  int id;
  CclsSemanticHighlightSymbol *symbol;
  Role role;
  bool operator<(const ScanLineEvent &o) const {
    // See the comments below when insertion/deletion events are inserted.
    if (!(pos == o.pos))
      return pos < o.pos;
    if (!(o.end_pos == end_pos))
      return o.end_pos < end_pos;
    // This comparison essentially order Macro after non-Macro,
    // So that macros will not be rendered as Var/Type/...
    if (symbol->kind != o.symbol->kind)
      return symbol->kind < o.symbol->kind;
    // If symbol A and B occupy the same place, we want one to be placed
    // before the other consistantly.
    return symbol->id < o.symbol->id;
  }
};

} // namespace
constexpr Position documentBegin{0, 0};
constexpr Position documentEnd{
    std::numeric_limits<decltype(Position::line)>::max(),
    std::numeric_limits<decltype(Position::character)>::max()};

inline std::ostream &operator<<(std::ostream &s, const Position pos) {
  s << "{line: " << pos.line << ", end: " << pos.character << "}";
  return s;
}
inline std::ostream &operator<<(std::ostream &s, const lsRange &range) {
  s << "lsRange(start:" << range.start << ", end:" << range.end << ")";
  return s;
}

void MessageHandler::textDocument_semanticTokensRange(
    SemanticTokensRangeParams &param, ReplyOnce &reply) {
  const std::string path = param.textDocument.uri.getPath();
  if (param.range.start == documentBegin && param.range.end == documentEnd)
    LOG_S(INFO) << "SemanticToken for all document of " << path;
  else
    LOG_S(INFO) << "SemanticToken for range " << param.range.start << " of " << path;

  WorkingFile *wfile = wfiles->getFile(path);
  if (!wfile) {
    reply.notOpened(path);
    return;
  }

  auto [queryFile, wFile] = findOrFail(path, reply);
  if (!queryFile) {
    // `findOrFail` already set the reply message
    return;
  }

  QueryFile::SemanticTokensWithId result;

  static GroupMatch match(g_config->highlight.whitelist,
                          g_config->highlight.blacklist);
  assert(queryFile->def);
  if (wfile->buffer_content.size() > g_config->highlight.largeFileSize ||
      !match.matches(queryFile->def->path)) {
    LOG_S(INFO) << "Not SemTokenizing " << path
                << "because of allowlist/denylist";
    return;
  }

  // Group symbols together.
  std::unordered_map<SymbolIdx, CclsSemanticHighlightSymbol> grouped_symbols;
  for (auto [sym, refcnt] : queryFile->symbol2refcnt) {
    if (refcnt <= 0)
      continue;
    // skip symbols that don't intersect range
    if (sym.range.end.line < param.range.start.line ||
        sym.range.start.line > param.range.end.line
        // range is within lines here below, let's test if within specified
        // characters/columns
        || sym.range.end.column < param.range.start.character ||
        sym.range.start.column > param.range.end.character)
      continue;
    std::string_view detailed_name;
    SymbolKind parent_kind = SymbolKind::Unknown;
    SymbolKind kind = SymbolKind::Unknown;
    uint8_t storage = SC_None;
    decltype(db->func_usr)::key_type idx;
    // This switch statement also filters out symbols that are not highlighted.
    switch (sym.kind) {
    case Kind::Func: {
      idx = db->func_usr[sym.usr];
      const QueryFunc &func = db->funcs[idx];
      const QueryFunc::Def *def = func.anyDef();
      if (!def)
        continue; // applies to for loop
      // Don't highlight overloadable operators or implicit lambda ->
      // std::function constructor.
      const auto short_name = def->name(false);
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
      const auto concise_name =
          detailed_name.substr(0, detailed_name.find('<'));
      const auto start_line_idx = sym.range.start.line;
      const auto start_col = sym.range.start.column;
      if (start_line_idx >= wfile->index_lines.size()) // out-of-range ?
        continue;
      const auto line = wfile->index_lines[start_line_idx];
      sym.range.end.line = start_line_idx;
      if (!(start_col + concise_name.size() <= line.size() &&
            line.compare(start_col, concise_name.size(), concise_name) == 0))
        continue;
      sym.range.end.column = start_col + concise_name.size();
      break;
    }
    case Kind::Type: {
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
    case Kind::Var: {
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

    if (auto maybe_loc = getLsRange(wfile, sym.range)) {
      auto it = grouped_symbols.find(sym);
      const auto &loc = *maybe_loc;
      if (it != grouped_symbols.end()) {
        it->second.lsRangeAndRoles.push_back({loc, sym.role});
      } else {
        CclsSemanticHighlightSymbol symbol;
        symbol.id = idx;
        symbol.parentKind = parent_kind;
        symbol.kind = kind;
        symbol.storage = storage;
        symbol.lsRangeAndRoles.push_back({loc, sym.role});
        grouped_symbols[sym] = symbol;
      }
    }
  }

  // Make ranges non-overlapping using a scan line algorithm.
  std::vector<ScanLineEvent> events;
  int id = 0;
  for (auto &entry : grouped_symbols) {
    CclsSemanticHighlightSymbol &symbol = entry.second;
    for (auto &loc : symbol.lsRangeAndRoles) {
      // For ranges sharing the same start point, the one with leftmost end
      // point comes first.
      events.push_back(
          {loc.first.start, loc.first.end, id, &symbol, loc.second});
      // For ranges sharing the same end point, their relative order does not
      // matter, therefore we arbitrarily assign loc.end to them. We use
      // negative id to indicate a deletion event.
      events.push_back(
          {loc.first.end, loc.first.end, ~id, &symbol, loc.second});
      id++;
    }
    symbol.lsRangeAndRoles.clear();
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
      events[top - 1].symbol->lsRangeAndRoles.push_back(
          {{events[i - 1].pos, events[i].pos}, events[i].role});
    if (events[i].id >= 0)
      events[top++] = events[i];
    else
      deleted[~events[i].id] = 1;
  }

  // Transform lsRange into pair<int, int> (offset pairs)
  std::vector<
      std::pair<std::pair<lsRange, Role>, CclsSemanticHighlightSymbol *>>
      scratch;
  for (auto &entry : grouped_symbols) {
    for (auto &range : entry.second.lsRangeAndRoles)
      scratch.emplace_back(range, &entry.second);
    entry.second.lsRangeAndRoles.clear();
  }
  std::sort(scratch.begin(), scratch.end(), [](auto &l, auto &r) {
    return l.first.first.start < r.first.first.start;
  });
  int line = 0;
  int column = 0;
  for (auto &entry : scratch) {
    lsRange &r = entry.first.first;
    if (r.start.line != line) {
      column = 0;
    }

    auto &serialized = result.tokens.data;

    serialized.push_back(r.start.line - line);
    line = r.start.line;
    serialized.push_back(r.start.character - column);
    column = r.start.character;
    serialized.push_back(r.end.character - r.start.character);

    uint8_t kindId;
    int modifiers = entry.second->storage == SC_Static ? 4 : 0;

    if (entry.first.second & Role::Declaration)
      modifiers |= 1;

    if (entry.first.second & Role::Definition)
      modifiers |= 2;

    if (entry.second->kind == SymbolKind::StaticMethod) {
      kindId = (uint8_t)SymbolKind::Method;
      modifiers = 4;
    } else {
      kindId = (uint8_t)entry.second->kind;
      if (kindId > (uint8_t)SymbolKind::StaticMethod)
        kindId--;
      if (kindId >= 252)
        kindId = 27 + kindId - 252;
    }
    serialized.push_back(kindId);
    serialized.push_back(modifiers);
  }
  // tokens ready, let's tag them with "the next id"
  result.id = queryFile->latestSemanticTokens.id + 1;
  // before sending data, we'll cache the token we're sending
  queryFile->latestSemanticTokens = result;

  reply(result.tokens);
}

void MessageHandler::textDocument_semanticTokensFull(
    SemanticTokensParams &param, ReplyOnce &reply) {
  lsRange fullRange{documentBegin, documentEnd};
  SemanticTokensRangeParams fullRangeParameters{param.textDocument, fullRange};
  textDocument_semanticTokensRange(fullRangeParameters, reply);
}

} // namespace ccls
