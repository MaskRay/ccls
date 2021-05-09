// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.hh"
#include "pipeline.hh"
#include "query.hh"

#include <algorithm>

MAKE_HASHABLE(ccls::SymbolIdx, t.usr, t.kind);

namespace ccls {
REFLECT_STRUCT(SymbolInformation, name, kind, location, containerName);

namespace {
struct DocumentHighlight {
  enum Kind { Text = 1, Read = 2, Write = 3 };

  lsRange range;
  int kind = 1;

  // ccls extension
  Role role = Role::None;

  bool operator<(const DocumentHighlight &o) const {
    return !(range == o.range) ? range < o.range : kind < o.kind;
  }
};
REFLECT_STRUCT(DocumentHighlight, range, kind, role);
} // namespace

void MessageHandler::textDocument_documentHighlight(
    TextDocumentPositionParam &param, ReplyOnce &reply) {
  int file_id;
  auto [file, wf] =
      findOrFail(param.textDocument.uri.getPath(), reply, &file_id);
  if (!wf)
    return;

  std::vector<DocumentHighlight> result;
  std::vector<SymbolRef> syms =
      findSymbolsAtLocation(wf, file, param.position, true);
  for (auto [sym, refcnt] : file->symbol2refcnt) {
    if (refcnt <= 0)
      continue;
    Usr usr = sym.usr;
    Kind kind = sym.kind;
    if (std::none_of(syms.begin(), syms.end(), [&](auto &sym1) {
          return usr == sym1.usr && kind == sym1.kind;
        }))
      continue;
    if (auto loc = getLsLocation(db, wfiles, sym, file_id)) {
      DocumentHighlight highlight;
      highlight.range = loc->range;
      if (sym.role & Role::Write)
        highlight.kind = DocumentHighlight::Write;
      else if (sym.role & Role::Read)
        highlight.kind = DocumentHighlight::Read;
      else
        highlight.kind = DocumentHighlight::Text;
      highlight.role = sym.role;
      result.push_back(highlight);
    }
  }
  std::sort(result.begin(), result.end());
  reply(result);
}

namespace {
struct DocumentLink {
  lsRange range;
  DocumentUri target;
};
REFLECT_STRUCT(DocumentLink, range, target);
} // namespace

void MessageHandler::textDocument_documentLink(TextDocumentParam &param,
                                               ReplyOnce &reply) {
  auto [file, wf] = findOrFail(param.textDocument.uri.getPath(), reply);
  if (!wf)
    return;

  std::vector<DocumentLink> result;
  int column;
  for (const IndexInclude &include : file->def->includes)
    if (std::optional<int> bline =
            wf->getBufferPosFromIndexPos(include.line, &column, false)) {
      const std::string &line = wf->buffer_lines[*bline];
      auto start = line.find_first_of("\"<"), end = line.find_last_of("\">");
      if (start < end)
        result.push_back({lsRange{{*bline, (int)start + 1}, {*bline, (int)end}},
                          DocumentUri::fromPath(include.resolved_path)});
    }
  reply(result);
} // namespace ccls

namespace {
struct DocumentSymbolParam : TextDocumentParam {
  // Include sym if `!(sym.role & excludeRole)`.
  Role excludeRole = Role((int)Role::All - (int)Role::Definition -
                          (int)Role::Declaration - (int)Role::Dynamic);
  // If >= 0, return Range[] instead of SymbolInformation[] to reduce output.
  int startLine = -1;
  int endLine = -1;
};
REFLECT_STRUCT(DocumentSymbolParam, textDocument, excludeRole, startLine,
               endLine);

struct DocumentSymbol {
  std::string name;
  std::string detail;
  SymbolKind kind;
  lsRange range;
  lsRange selectionRange;
  std::vector<std::unique_ptr<DocumentSymbol>> children;
};
void reflect(JsonWriter &vis, std::unique_ptr<DocumentSymbol> &v);
REFLECT_STRUCT(DocumentSymbol, name, detail, kind, range, selectionRange,
               children);
void reflect(JsonWriter &vis, std::unique_ptr<DocumentSymbol> &v) {
  reflect(vis, *v);
}

template <typename Def> bool ignore(const Def *def) { return false; }
template <> bool ignore(const QueryType::Def *def) {
  return !def || def->kind == SymbolKind::TypeParameter;
}
template <> bool ignore(const QueryVar::Def *def) {
  return !def || def->is_local();
}
} // namespace

void MessageHandler::textDocument_documentSymbol(JsonReader &reader,
                                                 ReplyOnce &reply) {
  DocumentSymbolParam param;
  reflect(reader, param);

  int file_id;
  auto [file, wf] =
      findOrFail(param.textDocument.uri.getPath(), reply, &file_id, true);
  if (!file)
    return;
  auto allows = [&](SymbolRef sym) { return !(sym.role & param.excludeRole); };

  if (param.startLine >= 0) {
    std::vector<lsRange> result;
    for (auto [sym, refcnt] : file->symbol2refcnt) {
      if (refcnt <= 0 || !allows(sym) ||
          !(param.startLine <= sym.range.start.line &&
            sym.range.start.line <= param.endLine))
        continue;
      if (auto loc = getLsLocation(db, wfiles, sym, file_id))
        result.push_back(loc->range);
    }
    std::sort(result.begin(), result.end());
    reply(result);
  } else if (g_config->client.hierarchicalDocumentSymbolSupport) {
    std::vector<ExtentRef> syms;
    syms.reserve(file->symbol2refcnt.size());
    for (auto [sym, refcnt] : file->symbol2refcnt)
      if (refcnt > 0 && sym.extent.valid())
        syms.push_back(sym);
    // Global variables `int i, j, k;` have the same extent.start. Sort them by
    // range.start instead. In case of a tie, prioritize the widest ExtentRef.
    std::sort(syms.begin(), syms.end(),
              [](const ExtentRef &lhs, const ExtentRef &rhs) {
                return std::tie(lhs.range.start, rhs.extent.end) <
                       std::tie(rhs.range.start, lhs.extent.end);
              });

    std::vector<std::unique_ptr<DocumentSymbol>> res;
    std::vector<DocumentSymbol *> scopes;
    for (ExtentRef sym : syms) {
      auto ds = std::make_unique<DocumentSymbol>();
      if (auto range = getLsRange(wf, sym.range)) {
        ds->selectionRange = *range;
        ds->range = ds->selectionRange;
        // For a macro expansion, M(name), we may use `M` for extent and
        // `name` for spell, do the check as selectionRange must be a subrange
        // of range.
        if (sym.extent.valid())
          if (auto range1 = getLsRange(wf, sym.extent);
              range1 && range1->includes(*range))
            ds->range = *range1;
      }
      withEntity(db, sym, [&](const auto &entity) {
        const auto *def = entity.anyDef();
        if (!def)
          return;
        ds->name = def->name(false);
        ds->detail = def->detailed_name;
        ds->kind = def->kind;

        if (!ignore(def) && (ds->kind == SymbolKind::Namespace || allows(sym))) {
          // Drop scopes which are before selectionRange.start. In
          // `int i, j, k;`, the scope of i will be ended by j.
          while (!scopes.empty() &&
                 scopes.back()->range.end <= ds->selectionRange.start)
            scopes.pop_back();
          auto *ds1 = ds.get();
          if (scopes.empty())
            res.push_back(std::move(ds));
          else
            scopes.back()->children.push_back(std::move(ds));
          scopes.push_back(ds1);
        }
      });
    }
    reply(res);
  } else {
    std::vector<SymbolInformation> result;
    for (auto [sym, refcnt] : file->symbol2refcnt) {
      if (refcnt <= 0 || !allows(sym))
        continue;
      if (std::optional<SymbolInformation> info =
              getSymbolInfo(db, sym, false)) {
        if ((sym.kind == Kind::Type && ignore(db->getType(sym).anyDef())) ||
            (sym.kind == Kind::Var && ignore(db->getVar(sym).anyDef())))
          continue;
        if (auto loc = getLsLocation(db, wfiles, sym, file_id)) {
          info->location = *loc;
          result.push_back(*info);
        }
      }
    }
    reply(result);
  }
}
} // namespace ccls
