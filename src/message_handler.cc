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

#include "message_handler.hh"

#include "log.hh"
#include "pipeline.hh"
#include "project.hh"
#include "query.hh"

#include <rapidjson/document.h>
#include <rapidjson/reader.h>

#include <algorithm>
#include <stdexcept>

using namespace clang;

MAKE_HASHABLE(ccls::SymbolIdx, t.usr, t.kind);

namespace ccls {
REFLECT_STRUCT(CodeActionParam::Context, diagnostics);
REFLECT_STRUCT(CodeActionParam, textDocument, range, context);
void Reflect(JsonReader &, EmptyParam &) {}
REFLECT_STRUCT(TextDocumentParam, textDocument);
REFLECT_STRUCT(DidOpenTextDocumentParam, textDocument);
REFLECT_STRUCT(TextDocumentContentChangeEvent, range, rangeLength, text);
REFLECT_STRUCT(TextDocumentDidChangeParam, textDocument, contentChanges);
REFLECT_STRUCT(TextDocumentPositionParam, textDocument, position);
REFLECT_STRUCT(RenameParam, textDocument, position, newName);

// completion
REFLECT_UNDERLYING(CompletionTriggerKind);
REFLECT_STRUCT(CompletionContext, triggerKind, triggerCharacter);
REFLECT_STRUCT(CompletionParam, textDocument, position, context);

// formatting
REFLECT_STRUCT(FormattingOptions, tabSize, insertSpaces);
REFLECT_STRUCT(DocumentFormattingParam, textDocument, options);
REFLECT_STRUCT(DocumentOnTypeFormattingParam, textDocument, position, ch,
               options);
REFLECT_STRUCT(DocumentRangeFormattingParam, textDocument, range, options);

// workspace
REFLECT_UNDERLYING(FileChangeType);
REFLECT_STRUCT(DidChangeWatchedFilesParam::Event, uri, type);
REFLECT_STRUCT(DidChangeWatchedFilesParam, changes);
REFLECT_STRUCT(DidChangeWorkspaceFoldersParam::Event, added, removed);
REFLECT_STRUCT(DidChangeWorkspaceFoldersParam, event);
REFLECT_STRUCT(WorkspaceSymbolParam, query, folders);

namespace {
struct CclsSemanticHighlightSymbol {
  int id = 0;
  SymbolKind parentKind;
  SymbolKind kind;
  uint8_t storage;
  std::vector<std::pair<int, int>> ranges;

  // `lsRanges` is used to compute `ranges`.
  std::vector<lsRange> lsRanges;
};

struct CclsSemanticHighlight {
  DocumentUri uri;
  std::vector<CclsSemanticHighlightSymbol> symbols;
};
REFLECT_STRUCT(CclsSemanticHighlightSymbol, id, parentKind, kind, storage,
               ranges, lsRanges);
REFLECT_STRUCT(CclsSemanticHighlight, uri, symbols);

struct CclsSetSkippedRanges {
  DocumentUri uri;
  std::vector<lsRange> skippedRanges;
};
REFLECT_STRUCT(CclsSetSkippedRanges, uri, skippedRanges);

struct ScanLineEvent {
  Position pos;
  Position end_pos; // Second key when there is a tie for insertion events.
  int id;
  CclsSemanticHighlightSymbol *symbol;
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

void ReplyOnce::NotReady(bool file) {
  if (file)
    Error(ErrorCode::InvalidRequest, "not opened");
  else
    Error(ErrorCode::InternalError, "not indexed");
}

void ReplyOnce::ReplyLocationLink(std::vector<LocationLink> &result) {
  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());
  if (result.size() > g_config->xref.maxNum)
    result.resize(g_config->xref.maxNum);
  if (g_config->client.linkSupport) {
    (*this)(result);
  } else {
    std::vector<Location> result1;
    for (auto &loc : result)
      result1.emplace_back(std::move(loc));
    (*this)(result1);
  }
}

void MessageHandler::Bind(const char *method,
                          void (MessageHandler::*handler)(JsonReader &)) {
  method2notification[method] = [this, handler](JsonReader &reader) {
    (this->*handler)(reader);
  };
}

template <typename Param>
void MessageHandler::Bind(const char *method,
                          void (MessageHandler::*handler)(Param &)) {
  method2notification[method] = [this, handler](JsonReader &reader) {
    Param param{};
    Reflect(reader, param);
    (this->*handler)(param);
  };
}

void MessageHandler::Bind(const char *method,
                          void (MessageHandler::*handler)(JsonReader &,
                                                          ReplyOnce &)) {
  method2request[method] = [this, handler](JsonReader &reader,
                                           ReplyOnce &reply) {
    (this->*handler)(reader, reply);
  };
}

template <typename Param>
void MessageHandler::Bind(const char *method,
                          void (MessageHandler::*handler)(Param &,
                                                          ReplyOnce &)) {
  method2request[method] = [this, handler](JsonReader &reader,
                                           ReplyOnce &reply) {
    Param param{};
    Reflect(reader, param);
    (this->*handler)(param, reply);
  };
}

MessageHandler::MessageHandler() {
  // clang-format off
  Bind("$ccls/call", &MessageHandler::ccls_call);
  Bind("$ccls/fileInfo", &MessageHandler::ccls_fileInfo);
  Bind("$ccls/info", &MessageHandler::ccls_info);
  Bind("$ccls/inheritance", &MessageHandler::ccls_inheritance);
  Bind("$ccls/member", &MessageHandler::ccls_member);
  Bind("$ccls/navigate", &MessageHandler::ccls_navigate);
  Bind("$ccls/reload", &MessageHandler::ccls_reload);
  Bind("$ccls/vars", &MessageHandler::ccls_vars);
  Bind("exit", &MessageHandler::exit);
  Bind("initialize", &MessageHandler::initialize);
  Bind("initialized", &MessageHandler::initialized);
  Bind("shutdown", &MessageHandler::shutdown);
  Bind("textDocument/codeAction", &MessageHandler::textDocument_codeAction);
  Bind("textDocument/codeLens", &MessageHandler::textDocument_codeLens);
  Bind("textDocument/completion", &MessageHandler::textDocument_completion);
  Bind("textDocument/declaration", &MessageHandler::textDocument_declaration);
  Bind("textDocument/definition", &MessageHandler::textDocument_definition);
  Bind("textDocument/didChange", &MessageHandler::textDocument_didChange);
  Bind("textDocument/didClose", &MessageHandler::textDocument_didClose);
  Bind("textDocument/didOpen", &MessageHandler::textDocument_didOpen);
  Bind("textDocument/didSave", &MessageHandler::textDocument_didSave);
  Bind("textDocument/documentHighlight", &MessageHandler::textDocument_documentHighlight);
  Bind("textDocument/documentLink", &MessageHandler::textDocument_documentLink);
  Bind("textDocument/documentSymbol", &MessageHandler::textDocument_documentSymbol);
  Bind("textDocument/foldingRange", &MessageHandler::textDocument_foldingRange);
  Bind("textDocument/formatting", &MessageHandler::textDocument_formatting);
  Bind("textDocument/hover", &MessageHandler::textDocument_hover);
  Bind("textDocument/implementation", &MessageHandler::textDocument_implementation);
  Bind("textDocument/onTypeFormatting", &MessageHandler::textDocument_onTypeFormatting);
  Bind("textDocument/rangeFormatting", &MessageHandler::textDocument_rangeFormatting);
  Bind("textDocument/references", &MessageHandler::textDocument_references);
  Bind("textDocument/rename", &MessageHandler::textDocument_rename);
  Bind("textDocument/signatureHelp", &MessageHandler::textDocument_signatureHelp);
  Bind("textDocument/typeDefinition", &MessageHandler::textDocument_typeDefinition);
  Bind("workspace/didChangeConfiguration", &MessageHandler::workspace_didChangeConfiguration);
  Bind("workspace/didChangeWatchedFiles", &MessageHandler::workspace_didChangeWatchedFiles);
  Bind("workspace/didChangeWorkspaceFolders", &MessageHandler::workspace_didChangeWorkspaceFolders);
  Bind("workspace/executeCommand", &MessageHandler::workspace_executeCommand);
  Bind("workspace/symbol", &MessageHandler::workspace_symbol);
  // clang-format on
}

void MessageHandler::Run(InMessage &msg) {
  rapidjson::Document &doc = *msg.document;
  rapidjson::Value param;
  auto it = doc.FindMember("params");
  if (it != doc.MemberEnd())
    param = it->value;
  JsonReader reader(&param);
  if (msg.id.Valid()) {
    ReplyOnce reply{msg.id};
    auto it = method2request.find(msg.method);
    if (it != method2request.end()) {
      try {
        it->second(reader, reply);
      } catch (std::invalid_argument &ex) {
        reply.Error(ErrorCode::InvalidParams,
                    "invalid params of " + msg.method + ": expected " +
                        ex.what() + " for " + reader.GetPath());
      } catch (...) {
        reply.Error(ErrorCode::InternalError, "failed to process " + msg.method);
      }
    } else {
      reply.Error(ErrorCode::MethodNotFound, "unknown request " + msg.method);
    }
  } else {
    auto it = method2notification.find(msg.method);
    if (it != method2notification.end())
      try {
        it->second(reader);
      } catch (...) {
        ShowMessageParam param{MessageType::Error,
                               std::string("failed to process ") + msg.method};
        pipeline::Notify(window_showMessage, param);
      }
  }
}

QueryFile *MessageHandler::FindFile(const std::string &path, int *out_file_id) {
  QueryFile *ret = nullptr;
  auto it = db->name2file_id.find(LowerPathIfInsensitive(path));
  if (it != db->name2file_id.end()) {
    QueryFile &file = db->files[it->second];
    if (file.def) {
      ret = &file;
      if (out_file_id)
        *out_file_id = it->second;
      return ret;
    }
  }
  if (out_file_id)
    *out_file_id = -1;
  return ret;
}

void EmitSkippedRanges(WorkingFile *wfile, QueryFile &file) {
  CclsSetSkippedRanges params;
  params.uri = DocumentUri::FromPath(wfile->filename);
  for (Range skipped : file.def->skipped_ranges)
    if (auto ls_skipped = GetLsRange(wfile, skipped))
      params.skippedRanges.push_back(*ls_skipped);
  pipeline::Notify("$ccls/publishSkippedRanges", params);
}

void EmitSemanticHighlight(DB *db, WorkingFile *wfile, QueryFile &file) {
  static GroupMatch match(g_config->highlight.whitelist,
                          g_config->highlight.blacklist);
  assert(file.def);
  if (wfile->buffer_content.size() > g_config->highlight.largeFileSize ||
      !match.Matches(file.def->path))
    return;

  // Group symbols together.
  std::unordered_map<SymbolIdx, CclsSemanticHighlightSymbol> grouped_symbols;
  for (auto &[sym, refcnt] : file.symbol2refcnt) {
    if (refcnt <= 0) continue;
    std::string_view detailed_name;
    SymbolKind parent_kind = SymbolKind::Unknown;
    SymbolKind kind = SymbolKind::Unknown;
    uint8_t storage = SC_None;
    int idx;
    // This switch statement also filters out symbols that are not highlighted.
    switch (sym.kind) {
    case Kind::Func: {
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

    if (std::optional<lsRange> loc = GetLsRange(wfile, sym.range)) {
      auto it = grouped_symbols.find(sym);
      if (it != grouped_symbols.end()) {
        it->second.lsRanges.push_back(*loc);
      } else {
        CclsSemanticHighlightSymbol symbol;
        symbol.id = idx;
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
    CclsSemanticHighlightSymbol &symbol = entry.second;
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

  CclsSemanticHighlight params;
  params.uri = DocumentUri::FromPath(wfile->filename);
  // Transform lsRange into pair<int, int> (offset pairs)
  if (!g_config->highlight.lsRanges) {
    std::vector<std::pair<lsRange, CclsSemanticHighlightSymbol *>> scratch;
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
      params.symbols.push_back(std::move(entry.second));
  pipeline::Notify("$ccls/publishSemanticHighlight", params);
}
} // namespace ccls
