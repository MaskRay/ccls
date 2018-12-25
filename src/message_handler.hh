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
#include "query.hh"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace ccls {
struct SemaManager;
struct VFS;
struct IncludeComplete;
struct Project;
struct WorkingFile;
struct WorkingFiles;

namespace pipeline {
void Reply(RequestId id, const std::function<void(JsonWriter &)> &fn);
void ReplyError(RequestId id, const std::function<void(JsonWriter &)> &fn);
}

struct CodeActionParam {
  TextDocumentIdentifier textDocument;
  lsRange range;
  struct Context {
    std::vector<Diagnostic> diagnostics;
  } context;
};
struct EmptyParam {};
struct DidOpenTextDocumentParam {
  TextDocumentItem textDocument;
};
struct RenameParam {
  TextDocumentIdentifier textDocument;
  Position position;
  std::string newName;
};
struct TextDocumentParam {
  TextDocumentIdentifier textDocument;
};
struct TextDocumentPositionParam {
  TextDocumentIdentifier textDocument;
  Position position;
};
struct TextDocumentEdit {
  VersionedTextDocumentIdentifier textDocument;
  std::vector<TextEdit> edits;
};
REFLECT_STRUCT(TextDocumentEdit, textDocument, edits);
struct WorkspaceEdit {
  std::vector<TextDocumentEdit> documentChanges;
};
REFLECT_STRUCT(WorkspaceEdit, documentChanges);

// completion
enum class CompletionTriggerKind {
  Invoked = 1,
  TriggerCharacter = 2,
  TriggerForIncompleteCompletions = 3,
};
struct CompletionContext {
  CompletionTriggerKind triggerKind = CompletionTriggerKind::Invoked;
  std::optional<std::string> triggerCharacter;
};
struct CompletionParam : TextDocumentPositionParam {
  CompletionContext context;
};
enum class CompletionItemKind {
  Text = 1,
  Method = 2,
  Function = 3,
  Constructor = 4,
  Field = 5,
  Variable = 6,
  Class = 7,
  Interface = 8,
  Module = 9,
  Property = 10,
  Unit = 11,
  Value = 12,
  Enum = 13,
  Keyword = 14,
  Snippet = 15,
  Color = 16,
  File = 17,
  Reference = 18,
  Folder = 19,
  EnumMember = 20,
  Constant = 21,
  Struct = 22,
  Event = 23,
  Operator = 24,
  TypeParameter = 25,
};
enum class InsertTextFormat {
  PlainText = 1,
  Snippet = 2
};
struct CompletionItem {
  std::string label;
  CompletionItemKind kind = CompletionItemKind::Text;
  std::string detail;
  std::string documentation;
  std::string sortText;
  std::string filterText;
  InsertTextFormat insertTextFormat = InsertTextFormat::PlainText;
  TextEdit textEdit;
  std::vector<TextEdit> additionalTextEdits;

  std::vector<std::string> parameters_;
  int score_;
  unsigned priority_;
  int quote_kind_ = 0;
};

// formatting
struct FormattingOptions {
  int tabSize;
  bool insertSpaces;
};
struct DocumentFormattingParam {
  TextDocumentIdentifier textDocument;
  FormattingOptions options;
};
struct DocumentOnTypeFormattingParam {
  TextDocumentIdentifier textDocument;
  Position position;
  std::string ch;
  FormattingOptions options;
};
struct DocumentRangeFormattingParam {
  TextDocumentIdentifier textDocument;
  lsRange range;
  FormattingOptions options;
};

// workspace
enum class FileChangeType {
  Created = 1,
  Changed = 2,
  Deleted = 3,
};
struct DidChangeWatchedFilesParam {
  struct Event {
    DocumentUri uri;
    FileChangeType type;
  };
  std::vector<Event> changes;
};
struct DidChangeWorkspaceFoldersParam {
  struct Event {
    std::vector<WorkspaceFolder> added, removed;
  } event;
};
struct WorkspaceSymbolParam {
  std::string query;

  // ccls extensions
  std::vector<std::string> folders;
};
REFLECT_STRUCT(WorkspaceFolder, uri, name);

inline void Reflect(JsonReader &visitor, DocumentUri &value) {
  Reflect(visitor, value.raw_uri);
}
inline void Reflect(JsonWriter &visitor, DocumentUri &value) {
  Reflect(visitor, value.raw_uri);
}

REFLECT_UNDERLYING(ErrorCode);
REFLECT_STRUCT(ResponseError, code, message);
REFLECT_STRUCT(Position, line, character);
REFLECT_STRUCT(lsRange, start, end);
REFLECT_STRUCT(Location, uri, range);
REFLECT_STRUCT(LocationLink, targetUri, targetRange, targetSelectionRange);
REFLECT_UNDERLYING_B(SymbolKind);
REFLECT_STRUCT(TextDocumentIdentifier, uri);
REFLECT_STRUCT(TextDocumentItem, uri, languageId, version, text);
REFLECT_STRUCT(TextEdit, range, newText);
REFLECT_STRUCT(VersionedTextDocumentIdentifier, uri, version);
REFLECT_STRUCT(Diagnostic, range, severity, code, source, message);
REFLECT_STRUCT(ShowMessageParam, type, message);
REFLECT_UNDERLYING_B(LanguageId);

struct NotIndexed {
  std::string path;
};
struct MessageHandler;

struct ReplyOnce {
  MessageHandler &handler;
  RequestId id;
  template <typename Res> void operator()(Res &&result) const {
    if (id.Valid())
      pipeline::Reply(id, [&](JsonWriter &w) { Reflect(w, result); });
  }
  void Error(ErrorCode code, std::string message) const {
    ResponseError err{code, std::move(message)};
    if (id.Valid())
      pipeline::ReplyError(id, [&](JsonWriter &w) { Reflect(w, err); });
  }
  void NotOpened(std::string_view path);
  void ReplyLocationLink(std::vector<LocationLink> &result);
};

struct MessageHandler {
  SemaManager *manager = nullptr;
  DB *db = nullptr;
  IncludeComplete *include_complete = nullptr;
  Project *project = nullptr;
  VFS *vfs = nullptr;
  WorkingFiles *wfiles = nullptr;

  llvm::StringMap<std::function<void(JsonReader &)>> method2notification;
  llvm::StringMap<std::function<void(JsonReader &, ReplyOnce &)>>
      method2request;
  bool overdue = false;

  MessageHandler();
  void Run(InMessage &msg);
  QueryFile *FindFile(const std::string &path, int *out_file_id = nullptr);
  std::pair<QueryFile *, WorkingFile *> FindOrFail(const std::string &path,
                                                   ReplyOnce &reply,
                                                   int *out_file_id = nullptr);

private:
  void Bind(const char *method, void (MessageHandler::*handler)(JsonReader &));
  template <typename Param>
  void Bind(const char *method, void (MessageHandler::*handler)(Param &));
  void Bind(const char *method,
            void (MessageHandler::*handler)(JsonReader &, ReplyOnce &));
  template <typename Param>
  void Bind(const char *method,
            void (MessageHandler::*handler)(Param &, ReplyOnce &));

  void ccls_call(JsonReader &, ReplyOnce &);
  void ccls_fileInfo(TextDocumentParam &, ReplyOnce &);
  void ccls_info(EmptyParam &, ReplyOnce &);
  void ccls_inheritance(JsonReader &, ReplyOnce &);
  void ccls_member(JsonReader &, ReplyOnce &);
  void ccls_navigate(JsonReader &, ReplyOnce &);
  void ccls_reload(JsonReader &);
  void ccls_vars(JsonReader &, ReplyOnce &);
  void exit(EmptyParam &);
  void initialize(JsonReader &, ReplyOnce &);
  void initialized(EmptyParam &);
  void shutdown(EmptyParam &, ReplyOnce &);
  void textDocument_codeAction(CodeActionParam &, ReplyOnce &);
  void textDocument_codeLens(TextDocumentParam &, ReplyOnce &);
  void textDocument_completion(CompletionParam &, ReplyOnce &);
  void textDocument_declaration(TextDocumentPositionParam &, ReplyOnce &);
  void textDocument_definition(TextDocumentPositionParam &, ReplyOnce &);
  void textDocument_didChange(TextDocumentDidChangeParam &);
  void textDocument_didClose(TextDocumentParam &);
  void textDocument_didOpen(DidOpenTextDocumentParam &);
  void textDocument_didSave(TextDocumentParam &);
  void textDocument_documentHighlight(TextDocumentPositionParam &, ReplyOnce &);
  void textDocument_documentLink(TextDocumentParam &, ReplyOnce &);
  void textDocument_documentSymbol(JsonReader &, ReplyOnce &);
  void textDocument_foldingRange(TextDocumentParam &, ReplyOnce &);
  void textDocument_formatting(DocumentFormattingParam &, ReplyOnce &);
  void textDocument_hover(TextDocumentPositionParam &, ReplyOnce &);
  void textDocument_implementation(TextDocumentPositionParam &, ReplyOnce &);
  void textDocument_onTypeFormatting(DocumentOnTypeFormattingParam &,
                                     ReplyOnce &);
  void textDocument_rangeFormatting(DocumentRangeFormattingParam &,
                                    ReplyOnce &);
  void textDocument_references(JsonReader &, ReplyOnce &);
  void textDocument_rename(RenameParam &, ReplyOnce &);
  void textDocument_signatureHelp(TextDocumentPositionParam &, ReplyOnce &);
  void textDocument_typeDefinition(TextDocumentPositionParam &, ReplyOnce &);
  void workspace_didChangeConfiguration(EmptyParam &);
  void workspace_didChangeWatchedFiles(DidChangeWatchedFilesParam &);
  void workspace_didChangeWorkspaceFolders(DidChangeWorkspaceFoldersParam &);
  void workspace_executeCommand(JsonReader &, ReplyOnce &);
  void workspace_symbol(WorkspaceSymbolParam &, ReplyOnce &);
};

void EmitSkippedRanges(WorkingFile *wfile, QueryFile &file);

void EmitSemanticHighlight(DB *db, WorkingFile *wfile, QueryFile &file);
} // namespace ccls
