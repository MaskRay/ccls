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
struct CompletionManager;
struct VFS;
struct IncludeComplete;
struct Project;
struct WorkingFile;
struct WorkingFiles;

namespace pipeline {
void Reply(RequestId id, const std::function<void(Writer &)> &fn);
void ReplyError(RequestId id, const std::function<void(Writer &)> &fn);
}

struct CodeActionParam {
  TextDocumentIdentifier textDocument;
  lsRange range;
  struct Context {
    std::vector<Diagnostic> diagnostics;
  } context;
};
struct EmptyParam {
  bool placeholder;
};
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
MAKE_REFLECT_STRUCT(TextDocumentEdit, textDocument, edits);
struct WorkspaceEdit {
  std::vector<TextDocumentEdit> documentChanges;
};
MAKE_REFLECT_STRUCT(WorkspaceEdit, documentChanges);

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
  std::optional<std::string> documentation;
  std::string sortText;
  std::optional<std::string> filterText;
  std::string insertText;
  InsertTextFormat insertTextFormat = InsertTextFormat::PlainText;
  TextEdit textEdit;
  std::vector<TextEdit> additionalTextEdits;

  std::vector<std::string> parameters_;
  int score_;
  unsigned priority_;
  bool use_angle_brackets_ = false;
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
MAKE_REFLECT_STRUCT(WorkspaceFolder, uri, name);

MAKE_REFLECT_TYPE_PROXY(ErrorCode);
MAKE_REFLECT_STRUCT(ResponseError, code, message);
MAKE_REFLECT_STRUCT(Position, line, character);
MAKE_REFLECT_STRUCT(lsRange, start, end);
MAKE_REFLECT_STRUCT(Location, uri, range);
MAKE_REFLECT_TYPE_PROXY(SymbolKind);
MAKE_REFLECT_STRUCT(TextDocumentIdentifier, uri);
MAKE_REFLECT_STRUCT(TextDocumentItem, uri, languageId, version, text);
MAKE_REFLECT_STRUCT(TextEdit, range, newText);
MAKE_REFLECT_STRUCT(VersionedTextDocumentIdentifier, uri, version);
MAKE_REFLECT_STRUCT(Diagnostic, range, severity, code, source, message);
MAKE_REFLECT_STRUCT(ShowMessageParam, type, message);
MAKE_REFLECT_TYPE_PROXY(LanguageId);

// TODO llvm 8 llvm::unique_function
template <typename Res>
using Callback = std::function<void(Res*)>;

struct ReplyOnce {
  RequestId id;
  template <typename Res> void operator()(Res &result) const {
    if (id.Valid())
      pipeline::Reply(id, [&](Writer &w) { Reflect(w, result); });
  }
  template <typename Err> void Error(Err &err) const {
    if (id.Valid())
      pipeline::ReplyError(id, [&](Writer &w) { Reflect(w, err); });
  }
};

struct MessageHandler {
  CompletionManager *clang_complete = nullptr;
  DB *db = nullptr;
  IncludeComplete *include_complete = nullptr;
  Project *project = nullptr;
  VFS *vfs = nullptr;
  WorkingFiles *wfiles = nullptr;

  llvm::StringMap<std::function<void(Reader &)>> method2notification;
  llvm::StringMap<std::function<void(Reader &, ReplyOnce &)>> method2request;

  MessageHandler();
  void Run(InMessage &msg);
  QueryFile *FindFile(ReplyOnce &reply, const std::string &path,
                      int *out_file_id = nullptr);

private:
  void Bind(const char *method, void (MessageHandler::*handler)(Reader &));
  template <typename Param>
  void Bind(const char *method, void (MessageHandler::*handler)(Param &));
  void Bind(const char *method,
            void (MessageHandler::*handler)(Reader &, ReplyOnce &));
  template <typename Param>
  void Bind(const char *method,
            void (MessageHandler::*handler)(Param &, ReplyOnce &));

  void ccls_call(Reader &, ReplyOnce &);
  void ccls_fileInfo(TextDocumentParam &, ReplyOnce &);
  void ccls_info(EmptyParam &, ReplyOnce &);
  void ccls_inheritance(Reader &, ReplyOnce &);
  void ccls_member(Reader &, ReplyOnce &);
  void ccls_navigate(Reader &, ReplyOnce &);
  void ccls_reload(Reader &);
  void ccls_vars(Reader &, ReplyOnce &);
  void exit(EmptyParam &);
  void initialize(Reader &, ReplyOnce &);
  void shutdown(EmptyParam &, ReplyOnce &);
  void textDocument_codeAction(CodeActionParam &, ReplyOnce &);
  void textDocument_codeLens(TextDocumentParam &, ReplyOnce &);
  void textDocument_completion(CompletionParam &, ReplyOnce &);
  void textDocument_definition(TextDocumentPositionParam &, ReplyOnce &);
  void textDocument_didChange(TextDocumentDidChangeParam &);
  void textDocument_didClose(TextDocumentParam &);
  void textDocument_didOpen(DidOpenTextDocumentParam &);
  void textDocument_didSave(TextDocumentParam &);
  void textDocument_documentHighlight(TextDocumentPositionParam &, ReplyOnce &);
  void textDocument_documentLink(TextDocumentParam &, ReplyOnce &);
  void textDocument_documentSymbol(Reader &, ReplyOnce &);
  void textDocument_foldingRange(TextDocumentParam &, ReplyOnce &);
  void textDocument_formatting(DocumentFormattingParam &, ReplyOnce &);
  void textDocument_hover(TextDocumentPositionParam &, ReplyOnce &);
  void textDocument_implementation(TextDocumentPositionParam &, ReplyOnce &);
  void textDocument_onTypeFormatting(DocumentOnTypeFormattingParam &,
                                     ReplyOnce &);
  void textDocument_rangeFormatting(DocumentRangeFormattingParam &,
                                    ReplyOnce &);
  void textDocument_references(Reader &, ReplyOnce &);
  void textDocument_rename(RenameParam &, ReplyOnce &);
  void textDocument_signatureHelp(TextDocumentPositionParam &, ReplyOnce &);
  void textDocument_typeDefinition(TextDocumentPositionParam &, ReplyOnce &);
  void workspace_didChangeConfiguration(EmptyParam &);
  void workspace_didChangeWatchedFiles(DidChangeWatchedFilesParam &);
  void workspace_didChangeWorkspaceFolders(DidChangeWorkspaceFoldersParam &);
  void workspace_executeCommand(Reader &, ReplyOnce &);
  void workspace_symbol(WorkspaceSymbolParam &, ReplyOnce &);
};

void EmitSkippedRanges(WorkingFile *wfile, QueryFile &file);

void EmitSemanticHighlight(DB *db, WorkingFile *wfile, QueryFile &file);
} // namespace ccls
