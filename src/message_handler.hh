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
#include "query.h"

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace ccls {
struct CompletionManager;
struct Config;
struct GroupMatch;
struct VFS;
struct IncludeComplete;
struct MultiQueueWaiter;
struct Project;
struct DB;
struct WorkingFile;
struct WorkingFiles;

namespace pipeline {
void Reply(lsRequestId id, const std::function<void(Writer &)> &fn);
void ReplyError(lsRequestId id, const std::function<void(Writer &)> &fn);
}

struct EmptyParam {
  bool placeholder;
};
struct TextDocumentParam {
  lsTextDocumentIdentifier textDocument;
};
struct DidOpenTextDocumentParam {
  lsTextDocumentItem textDocument;
};

struct TextDocumentPositionParam {
  lsTextDocumentIdentifier textDocument;
  lsPosition position;
};

struct RenameParam {
  lsTextDocumentIdentifier textDocument;
  lsPosition position;
  std::string newName;
};

// code*
struct CodeActionParam {
  lsTextDocumentIdentifier textDocument;
  lsRange range;
  struct Context {
    std::vector<lsDiagnostic> diagnostics;
  } context;
};

// completion
enum class lsCompletionTriggerKind {
  Invoked = 1,
  TriggerCharacter = 2,
  TriggerForIncompleteCompletions = 3,
};
struct lsCompletionContext {
  lsCompletionTriggerKind triggerKind = lsCompletionTriggerKind::Invoked;
  std::optional<std::string> triggerCharacter;
};
struct lsCompletionParams : TextDocumentPositionParam {
  lsCompletionContext context;
};
enum class lsCompletionItemKind {
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
enum class lsInsertTextFormat {
  PlainText = 1,
  Snippet = 2
};
struct lsCompletionItem {
  std::string label;
  lsCompletionItemKind kind = lsCompletionItemKind::Text;
  std::string detail;
  std::optional<std::string> documentation;
  std::string sortText;
  std::optional<std::string> filterText;
  std::string insertText;
  lsInsertTextFormat insertTextFormat = lsInsertTextFormat::PlainText;
  lsTextEdit textEdit;
  std::vector<lsTextEdit> additionalTextEdits;

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
  lsTextDocumentIdentifier textDocument;
  FormattingOptions options;
};
struct DocumentOnTypeFormattingParam {
  lsTextDocumentIdentifier textDocument;
  lsPosition position;
  std::string ch;
  FormattingOptions options;
};
struct DocumentRangeFormattingParam {
  lsTextDocumentIdentifier textDocument;
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
    lsDocumentUri uri;
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
};

// TODO llvm 8 llvm::unique_function
template <typename Res>
using Callback = std::function<void(Res*)>;

struct ReplyOnce {
  lsRequestId id;
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
  DB *db = nullptr;
  Project *project = nullptr;
  VFS *vfs = nullptr;
  WorkingFiles *working_files = nullptr;
  CompletionManager *clang_complete = nullptr;
  IncludeComplete *include_complete = nullptr;

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
  void textDocument_completion(lsCompletionParams &, ReplyOnce &);
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
