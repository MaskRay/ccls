// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

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
void reflect(JsonReader &, EmptyParam &) {}
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

struct CclsSetSkippedRanges {
  DocumentUri uri;
  std::vector<lsRange> skippedRanges;
};
REFLECT_STRUCT(CclsSetSkippedRanges, uri, skippedRanges);

} // namespace

void ReplyOnce::notOpened(std::string_view path) {
  error(ErrorCode::InvalidRequest, std::string(path) + " is not opened");
}

void ReplyOnce::replyLocationLink(std::vector<LocationLink> &result) {
  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());
  if (result.size() > g_config->xref.maxNum)
    result.resize(g_config->xref.maxNum);
  if (g_config->client.linkSupport) {
    (*this)(result);
  } else {
    (*this)(std::vector<Location>(std::make_move_iterator(result.begin()),
                                  std::make_move_iterator(result.end())));
  }
}

void MessageHandler::bind(const char *method,
                          void (MessageHandler::*handler)(JsonReader &)) {
  method2notification[method] = [this, handler](JsonReader &reader) {
    (this->*handler)(reader);
  };
}

template <typename Param>
void MessageHandler::bind(const char *method,
                          void (MessageHandler::*handler)(Param &)) {
  method2notification[method] = [this, handler](JsonReader &reader) {
    Param param{};
    reflect(reader, param);
    (this->*handler)(param);
  };
}

void MessageHandler::bind(const char *method,
                          void (MessageHandler::*handler)(JsonReader &,
                                                          ReplyOnce &)) {
  method2request[method] = [this, handler](JsonReader &reader,
                                           ReplyOnce &reply) {
    (this->*handler)(reader, reply);
  };
}

template <typename Param>
void MessageHandler::bind(const char *method,
                          void (MessageHandler::*handler)(Param &,
                                                          ReplyOnce &)) {
  method2request[method] = [this, handler](JsonReader &reader,
                                           ReplyOnce &reply) {
    Param param{};
    reflect(reader, param);
    (this->*handler)(param, reply);
  };
}

MessageHandler::MessageHandler() {
  // clang-format off
  bind("$ccls/call", &MessageHandler::ccls_call);
  bind("$ccls/fileInfo", &MessageHandler::ccls_fileInfo);
  bind("$ccls/info", &MessageHandler::ccls_info);
  bind("$ccls/inheritance", &MessageHandler::ccls_inheritance);
  bind("$ccls/member", &MessageHandler::ccls_member);
  bind("$ccls/navigate", &MessageHandler::ccls_navigate);
  bind("$ccls/reload", &MessageHandler::ccls_reload);
  bind("$ccls/vars", &MessageHandler::ccls_vars);
  bind("exit", &MessageHandler::exit);
  bind("initialize", &MessageHandler::initialize);
  bind("initialized", &MessageHandler::initialized);
  bind("shutdown", &MessageHandler::shutdown);
  bind("textDocument/codeAction", &MessageHandler::textDocument_codeAction);
  bind("textDocument/codeLens", &MessageHandler::textDocument_codeLens);
  bind("textDocument/completion", &MessageHandler::textDocument_completion);
  bind("textDocument/declaration", &MessageHandler::textDocument_declaration);
  bind("textDocument/definition", &MessageHandler::textDocument_definition);
  bind("textDocument/didChange", &MessageHandler::textDocument_didChange);
  bind("textDocument/didClose", &MessageHandler::textDocument_didClose);
  bind("textDocument/didOpen", &MessageHandler::textDocument_didOpen);
  bind("textDocument/didSave", &MessageHandler::textDocument_didSave);
  bind("textDocument/documentHighlight", &MessageHandler::textDocument_documentHighlight);
  bind("textDocument/documentLink", &MessageHandler::textDocument_documentLink);
  bind("textDocument/documentSymbol", &MessageHandler::textDocument_documentSymbol);
  bind("textDocument/foldingRange", &MessageHandler::textDocument_foldingRange);
  bind("textDocument/formatting", &MessageHandler::textDocument_formatting);
  bind("textDocument/hover", &MessageHandler::textDocument_hover);
  bind("textDocument/implementation", &MessageHandler::textDocument_implementation);
  bind("textDocument/onTypeFormatting", &MessageHandler::textDocument_onTypeFormatting);
  bind("textDocument/rangeFormatting", &MessageHandler::textDocument_rangeFormatting);
  bind("textDocument/references", &MessageHandler::textDocument_references);
  bind("textDocument/rename", &MessageHandler::textDocument_rename);
  bind("textDocument/signatureHelp", &MessageHandler::textDocument_signatureHelp);
  bind("textDocument/typeDefinition", &MessageHandler::textDocument_typeDefinition);
  bind("textDocument/semanticTokens/full", &MessageHandler::textDocument_semanticTokensFull);
  bind("textDocument/semanticTokens/range", &MessageHandler::textDocument_semanticTokensRange);
  bind("workspace/didChangeConfiguration", &MessageHandler::workspace_didChangeConfiguration);
  bind("workspace/didChangeWatchedFiles", &MessageHandler::workspace_didChangeWatchedFiles);
  bind("workspace/didChangeWorkspaceFolders", &MessageHandler::workspace_didChangeWorkspaceFolders);
  bind("workspace/executeCommand", &MessageHandler::workspace_executeCommand);
  bind("workspace/symbol", &MessageHandler::workspace_symbol);
  // clang-format on
}

void MessageHandler::run(InMessage &msg) {
  rapidjson::Document &doc = *msg.document;
  rapidjson::Value null;
  auto it = doc.FindMember("params");
  JsonReader reader(it != doc.MemberEnd() ? &it->value : &null);
  if (msg.id.valid()) {
    ReplyOnce reply{*this, msg.id};
    auto it = method2request.find(msg.method);
    if (it != method2request.end()) {
      try {
        it->second(reader, reply);
      } catch (std::invalid_argument &ex) {
        reply.error(ErrorCode::InvalidParams,
                    "invalid params of " + msg.method + ": expected " +
                        ex.what() + " for " + reader.getPath());
      } catch (NotIndexed &) {
        throw;
      } catch (...) {
        reply.error(ErrorCode::InternalError,
                    "failed to process " + msg.method);
      }
    } else {
      reply.error(ErrorCode::MethodNotFound, "unknown request " + msg.method);
    }
  } else {
    auto it = method2notification.find(msg.method);
    if (it != method2notification.end())
      try {
        it->second(reader);
      } catch (...) {
        ShowMessageParam param{MessageType::Error,
                               std::string("failed to process ") + msg.method};
        pipeline::notify(window_showMessage, param);
      }
  }
}

QueryFile *MessageHandler::findFile(const std::string &path, int *out_file_id) {
  QueryFile *ret = nullptr;
  auto it = db->name2file_id.find(lowerPathIfInsensitive(path));
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

std::pair<QueryFile *, WorkingFile *>
MessageHandler::findOrFail(const std::string &path, ReplyOnce &reply,
                           int *out_file_id, bool allow_unopened) {
  WorkingFile *wf = wfiles->getFile(path);
  if (!wf && !allow_unopened) {
    reply.notOpened(path);
    return {nullptr, nullptr};
  }
  QueryFile *file = findFile(path, out_file_id);
  if (!file) {
    if (!overdue)
      throw NotIndexed{path};
    reply.error(ErrorCode::InvalidRequest, "not indexed");
    return {nullptr, nullptr};
  }
  return {file, wf};
}

void emitSkippedRanges(WorkingFile *wfile, QueryFile &file) {
  CclsSetSkippedRanges params;
  params.uri = DocumentUri::fromPath(wfile->filename);
  for (Range skipped : file.def->skipped_ranges)
    if (auto ls_skipped = getLsRange(wfile, skipped))
      params.skippedRanges.push_back(*ls_skipped);
  pipeline::notify("$ccls/publishSkippedRanges", params);
}


void emitSemanticHighlightRefresh() {
  //// tried using `notify`, but won't compile
  //EmptyParam empty;
  //pipeline::notify("workspace/semanticTokens/refresh", empty);
  
  pipeline::notifyOrRequest(
   "workspace/semanticTokens/refresh", false, [](JsonWriter &){});
}
} // namespace ccls
