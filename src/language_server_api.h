#pragma once

#include "config.h"
#include "ipc.h"
#include "serializer.h"
#include "serializers/json.h"
#include "utils.h"

#include <optional.h>
#include <rapidjson/writer.h>
#include <variant.h>

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////////////////// OUTGOING MESSAGES /////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////////////////// INCOMING MESSAGES /////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define REGISTER_IPC_MESSAGE(type) \
  static MessageRegistryRegister<type> type##message_handler_instance_;

struct MessageRegistry {
  static MessageRegistry* instance_;
  static MessageRegistry* instance();

  using Allocator =
      std::function<void(Reader& visitor, std::unique_ptr<BaseIpcMessage>*)>;
  std::unordered_map<std::string, Allocator> allocators;

  optional<std::string> ReadMessageFromStdin(
      std::unique_ptr<BaseIpcMessage>* message);
  optional<std::string> Parse(Reader& visitor,
                              std::unique_ptr<BaseIpcMessage>* message);
};

template <typename T>
struct MessageRegistryRegister {
  MessageRegistryRegister() {
    std::string method_name = IpcIdToString(T::kIpcId);
    MessageRegistry::instance()->allocators[method_name] =
        [](Reader& visitor, std::unique_ptr<BaseIpcMessage>* message) {
          *message = MakeUnique<T>();
          // Reflect may throw and *message will be partially deserialized.
          Reflect(visitor, static_cast<T&>(**message));
        };
  }
};

struct lsBaseOutMessage {
  virtual ~lsBaseOutMessage();
  virtual void Write(std::ostream& out) = 0;
};

template <typename TDerived>
struct lsOutMessage : lsBaseOutMessage {
  // All derived types need to reflect on the |jsonrpc| member.
  std::string jsonrpc = "2.0";

  // Send the message to the language client by writing it to stdout.
  void Write(std::ostream& out) override {
    rapidjson::StringBuffer output;
    rapidjson::Writer<rapidjson::StringBuffer> writer(output);
    JsonWriter json_writer(&writer);
    auto that = static_cast<TDerived*>(this);
    Reflect(json_writer, *that);

    out << "Content-Length: " << output.GetSize();
    out << (char)13 << char(10) << char(13) << char(10);  // CRLFCRLF
    out << output.GetString();
    out.flush();
  }
};

struct lsResponseError {
  struct Data {
    virtual void Write(Writer& writer) = 0;
  };

  enum class lsErrorCodes : int {
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    serverErrorStart = -32099,
    serverErrorEnd = -32000,
    ServerNotInitialized = -32002,
    UnknownErrorCode = -32001
  };

  lsErrorCodes code;
  // Short description.
  std::string message;
  std::unique_ptr<Data> data;

  void Write(Writer& visitor);
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////////////////////////// PRIMITIVE TYPES //////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct lsDocumentUri {
  static lsDocumentUri FromPath(const std::string& path);

  lsDocumentUri();
  bool operator==(const lsDocumentUri& other) const;

  void SetPath(const std::string& path);
  std::string GetPath() const;

  std::string raw_uri;
};
MAKE_HASHABLE(lsDocumentUri, t.raw_uri);

template <typename TVisitor>
void Reflect(TVisitor& visitor, lsDocumentUri& value) {
  Reflect(visitor, value.raw_uri);
}

struct lsPosition {
  lsPosition();
  lsPosition(int line, int character);

  bool operator==(const lsPosition& other) const;
  bool operator<(const lsPosition& other) const;

  std::string ToString() const;

  // Note: these are 0-based.
  int line = 0;
  int character = 0;
  static const lsPosition kZeroPosition;
};
MAKE_HASHABLE(lsPosition, t.line, t.character);
MAKE_REFLECT_STRUCT(lsPosition, line, character);

struct lsRange {
  lsRange();
  lsRange(lsPosition start, lsPosition end);

  bool operator==(const lsRange& other) const;

  lsPosition start;
  lsPosition end;
};
MAKE_HASHABLE(lsRange, t.start, t.end);
MAKE_REFLECT_STRUCT(lsRange, start, end);

struct lsLocation {
  lsLocation();
  lsLocation(lsDocumentUri uri, lsRange range);

  bool operator==(const lsLocation& other) const;

  lsDocumentUri uri;
  lsRange range;
};
MAKE_HASHABLE(lsLocation, t.uri, t.range);
MAKE_REFLECT_STRUCT(lsLocation, uri, range);

enum class lsSymbolKind : int {
  File = 1,
  Module = 2,
  Namespace = 3,
  Package = 4,
  Class = 5,
  Method = 6,
  Property = 7,
  Field = 8,
  Constructor = 9,
  Enum = 10,
  Interface = 11,
  Function = 12,
  Variable = 13,
  Constant = 14,
  String = 15,
  Number = 16,
  Boolean = 17,
  Array = 18
};
MAKE_REFLECT_TYPE_PROXY(lsSymbolKind);

struct lsSymbolInformation {
  std::string_view name;
  lsSymbolKind kind;
  lsLocation location;
  std::string_view containerName;
};
MAKE_REFLECT_STRUCT(lsSymbolInformation, name, kind, location, containerName);

template <typename T>
struct lsCommand {
  // Title of the command (ie, 'save')
  std::string title;
  // Actual command identifier.
  std::string command;
  // Arguments to run the command with.
  // **NOTE** This must be serialized as an array. Use
  // MAKE_REFLECT_STRUCT_WRITER_AS_ARRAY.
  T arguments;
};
template <typename TVisitor, typename T>
void Reflect(TVisitor& visitor, lsCommand<T>& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(title);
  REFLECT_MEMBER(command);
  REFLECT_MEMBER(arguments);
  REFLECT_MEMBER_END();
}

template <typename TData, typename TCommandArguments>
struct lsCodeLens {
  // The range in which this code lens is valid. Should only span a single line.
  lsRange range;
  // The command this code lens represents.
  optional<lsCommand<TCommandArguments>> command;
  // A data entry field that is preserved on a code lens item between
  // a code lens and a code lens resolve request.
  TData data;
};
template <typename TVisitor, typename TData, typename TCommandArguments>
void Reflect(TVisitor& visitor, lsCodeLens<TData, TCommandArguments>& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(range);
  REFLECT_MEMBER(command);
  REFLECT_MEMBER(data);
  REFLECT_MEMBER_END();
}

struct lsTextDocumentIdentifier {
  lsDocumentUri uri;
};
MAKE_REFLECT_STRUCT(lsTextDocumentIdentifier, uri);

struct lsVersionedTextDocumentIdentifier {
  lsDocumentUri uri;
  // The version number of this document.  number | null
  std::variant<std::monostate, int> version;

  lsTextDocumentIdentifier AsTextDocumentIdentifier() const;
};
MAKE_REFLECT_STRUCT(lsVersionedTextDocumentIdentifier, uri, version);

struct lsTextDocumentPositionParams {
  // The text document.
  lsTextDocumentIdentifier textDocument;

  // The position inside the text document.
  lsPosition position;
};
MAKE_REFLECT_STRUCT(lsTextDocumentPositionParams, textDocument, position);

struct lsTextEdit {
  // The range of the text document to be manipulated. To insert
  // text into a document create a range where start === end.
  lsRange range;

  // The string to be inserted. For delete operations use an
  // empty string.
  std::string newText;

  bool operator==(const lsTextEdit& that);
};
MAKE_REFLECT_STRUCT(lsTextEdit, range, newText);

// Defines whether the insert text in a completion item should be interpreted as
// plain text or a snippet.
enum class lsInsertTextFormat {
  // The primary text to be inserted is treated as a plain string.
  PlainText = 1,

  // The primary text to be inserted is treated as a snippet.
  //
  // A snippet can define tab stops and placeholders with `$1`, `$2`
  // and `${3:foo}`. `$0` defines the final tab stop, it defaults to
  // the end of the snippet. Placeholders with equal identifiers are linked,
  // that is typing in one will update others too.
  //
  // See also:
  // https://github.com/Microsoft/vscode/blob/master/src/vs/editor/contrib/snippet/common/snippet.md
  Snippet = 2
};
MAKE_REFLECT_TYPE_PROXY(lsInsertTextFormat);

// The kind of a completion entry.
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
MAKE_REFLECT_TYPE_PROXY(lsCompletionItemKind);

struct lsCompletionItem {
  // A set of function parameters. Used internally for signature help. Not sent
  // to vscode.
  std::vector<std::string> parameters_;

  // The label of this completion item. By default
  // also the text that is inserted when selecting
  // this completion.
  std::string label;

  // The kind of this completion item. Based of the kind
  // an icon is chosen by the editor.
  lsCompletionItemKind kind = lsCompletionItemKind::Text;

  // A human-readable string with additional information
  // about this item, like type or symbol information.
  std::string detail;

  // A human-readable string that represents a doc-comment.
  std::string documentation;

  // Internal information to order candidates.
  bool found_;
  std::string::size_type skip_;
  unsigned priority_;

  // A string that shoud be used when comparing this item
  // with other items. When `falsy` the label is used.
  std::string sortText;

  // A string that should be used when filtering a set of
  // completion items. When `falsy` the label is used.
  std::string filterText;

  // A string that should be inserted a document when selecting
  // this completion. When `falsy` the label is used.
  std::string insertText;

  // The format of the insert text. The format applies to both the `insertText`
  // property and the `newText` property of a provided `textEdit`.
  lsInsertTextFormat insertTextFormat = lsInsertTextFormat::PlainText;

  // An edit which is applied to a document when selecting this completion. When
  // an edit is provided the value of `insertText` is ignored.
  //
  // *Note:* The range of the edit must be a single line range and it must
  // contain the position at which completion has been requested.
  optional<lsTextEdit> textEdit;

  // An optional array of additional text edits that are applied when
  // selecting this completion. Edits must not overlap with the main edit
  // nor with themselves.
  // std::vector<TextEdit> additionalTextEdits;

  // An optional command that is executed *after* inserting this completion.
  // *Note* that additional modifications to the current document should be
  // described with the additionalTextEdits-property. Command command;

  // An data entry field that is preserved on a completion item between
  // a completion and a completion resolve request.
  // data ? : any

  // Use this helper to figure out what content the completion item will insert
  // into the document, as it could live in either |textEdit|, |insertText|, or
  // |label|.
  const std::string& InsertedContent() const;
};
MAKE_REFLECT_STRUCT(lsCompletionItem,
                    label,
                    kind,
                    detail,
                    documentation,
                    sortText,
                    insertText,
                    filterText,
                    insertTextFormat,
                    textEdit);

struct lsTextDocumentItem {
  // The text document's URI.
  lsDocumentUri uri;

  // The text document's language identifier.
  std::string languageId;

  // The version number of this document (it will strictly increase after each
  // change, including undo/redo).
  int version;

  // The content of the opened text document.
  std::string text;
};
MAKE_REFLECT_STRUCT(lsTextDocumentItem, uri, languageId, version, text);

struct lsTextDocumentEdit {
  // The text document to change.
  lsVersionedTextDocumentIdentifier textDocument;

  // The edits to be applied.
  std::vector<lsTextEdit> edits;
};
MAKE_REFLECT_STRUCT(lsTextDocumentEdit, textDocument, edits);

struct lsWorkspaceEdit {
  // Holds changes to existing resources.
  // changes ? : { [uri:string]: TextEdit[]; };
  // std::unordered_map<lsDocumentUri, std::vector<lsTextEdit>> changes;

  // An array of `TextDocumentEdit`s to express changes to specific a specific
  // version of a text document. Whether a client supports versioned document
  // edits is expressed via `WorkspaceClientCapabilites.versionedWorkspaceEdit`.
  std::vector<lsTextDocumentEdit> documentChanges;
};
MAKE_REFLECT_STRUCT(lsWorkspaceEdit, documentChanges);

// A document highlight kind.
enum class lsDocumentHighlightKind {
  // A textual occurrence.
  Text = 1,
  // Read-access of a symbol, like reading a variable.
  Read = 2,
  // Write-access of a symbol, like writing to a variable.
  Write = 3
};
MAKE_REFLECT_TYPE_PROXY(lsDocumentHighlightKind);

struct lsFormattingOptions {
  // Size of a tab in spaces.
  int tabSize;
  // Prefer spaces over tabs.
  bool insertSpaces;
};
MAKE_REFLECT_STRUCT(lsFormattingOptions, tabSize, insertSpaces);

// A document highlight is a range inside a text document which deserves
// special attention. Usually a document highlight is visualized by changing
// the background color of its range.
struct lsDocumentHighlight {
  // The range this highlight applies to.
  lsRange range;

  // The highlight kind, default is DocumentHighlightKind.Text.
  lsDocumentHighlightKind kind = lsDocumentHighlightKind::Text;
};
MAKE_REFLECT_STRUCT(lsDocumentHighlight, range, kind);

enum class lsDiagnosticSeverity {
  // Reports an error.
  Error = 1,
  // Reports a warning.
  Warning = 2,
  // Reports an information.
  Information = 3,
  // Reports a hint.
  Hint = 4
};
MAKE_REFLECT_TYPE_PROXY(lsDiagnosticSeverity);

struct lsDiagnostic {
  // The range at which the message applies.
  lsRange range;

  // The diagnostic's severity. Can be omitted. If omitted it is up to the
  // client to interpret diagnostics as error, warning, info or hint.
  optional<lsDiagnosticSeverity> severity;

  // The diagnostic's code. Can be omitted.
  int code = 0;

  // A human-readable string describing the source of this
  // diagnostic, e.g. 'typescript' or 'super lint'.
  std::string source = "cquery";

  // The diagnostic's message.
  std::string message;

  // Non-serialized set of fixits.
  std::vector<lsTextEdit> fixits_;
};
MAKE_REFLECT_STRUCT(lsDiagnostic, range, severity, source, message);

enum class lsErrorCodes {
  // Defined by JSON RPC
  ParseError = -32700,
  InvalidRequest = -32600,
  MethodNotFound = -32601,
  InvalidParams = -32602,
  InternalError = -32603,
  serverErrorStart = -32099,
  serverErrorEnd = -32000,
  ServerNotInitialized = -32002,
  UnknownErrorCode = -32001,

  // Defined by the protocol.
  RequestCancelled = -32800,
};
MAKE_REFLECT_TYPE_PROXY(lsErrorCodes);
struct Out_Error : public lsOutMessage<Out_Error> {
  struct lsResponseError {
    // A number indicating the error type that occurred.
    lsErrorCodes code;

    // A string providing a short description of the error.
    std::string message;

    // A Primitive or Structured value that contains additional
    // information about the error. Can be omitted.
    // optional<D> data;
  };

  lsRequestId id;

  // The error object in case a request fails.
  lsResponseError error;
};
MAKE_REFLECT_STRUCT(Out_Error::lsResponseError, code, message);
MAKE_REFLECT_STRUCT(Out_Error, jsonrpc, id, error);

// Cancel an existing request.
struct Ipc_CancelRequest : public RequestMessage<Ipc_CancelRequest> {
  static const IpcId kIpcId = IpcId::CancelRequest;
};
MAKE_REFLECT_STRUCT(Ipc_CancelRequest, id);

// Diagnostics
struct Out_TextDocumentPublishDiagnostics
    : public lsOutMessage<Out_TextDocumentPublishDiagnostics> {
  struct Params {
    // The URI for which diagnostic information is reported.
    lsDocumentUri uri;

    // An array of diagnostic information items.
    std::vector<lsDiagnostic> diagnostics;
  };

  Params params;
};
template <typename TVisitor>
void Reflect(TVisitor& visitor, Out_TextDocumentPublishDiagnostics& value) {
  std::string method = "textDocument/publishDiagnostics";
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(jsonrpc);
  REFLECT_MEMBER2("method", method);
  REFLECT_MEMBER(params);
  REFLECT_MEMBER_END();
}
MAKE_REFLECT_STRUCT(Out_TextDocumentPublishDiagnostics::Params,
                    uri,
                    diagnostics);

// MarkedString can be used to render human readable text. It is either a
// markdown string or a code-block that provides a language and a code snippet.
// The language identifier is sematically equal to the optional language
// identifier in fenced code blocks in GitHub issues. See
// https://help.github.com/articles/creating-and-highlighting-code-blocks/#syntax-highlighting
//
// The pair of a language and a value is an equivalent to markdown:
// ```${language}
// ${value}
// ```
//
// Note that markdown strings will be sanitized - that means html will be
// escaped.
struct lsMarkedString1 {
  std::string_view language;
  std::string_view value;
};
using lsMarkedString = std::variant<std::string_view, lsMarkedString1>;
MAKE_REFLECT_STRUCT(lsMarkedString1, language, value);

struct lsTextDocumentContentChangeEvent {
  // The range of the document that changed.
  optional<lsRange> range;
  // The length of the range that got replaced.
  optional<int> rangeLength;
  // The new text of the range/document.
  std::string text;
};
MAKE_REFLECT_STRUCT(lsTextDocumentContentChangeEvent, range, rangeLength, text);

struct lsTextDocumentDidChangeParams {
  lsVersionedTextDocumentIdentifier textDocument;
  std::vector<lsTextDocumentContentChangeEvent> contentChanges;
};
MAKE_REFLECT_STRUCT(lsTextDocumentDidChangeParams,
                    textDocument,
                    contentChanges);

// Show a message to the user.
enum class lsMessageType : int { Error = 1, Warning = 2, Info = 3, Log = 4 };
MAKE_REFLECT_TYPE_PROXY(lsMessageType)
struct Out_ShowLogMessageParams {
  lsMessageType type = lsMessageType::Error;
  std::string message;
};
MAKE_REFLECT_STRUCT(Out_ShowLogMessageParams, type, message);
struct Out_ShowLogMessage : public lsOutMessage<Out_ShowLogMessage> {
  enum class DisplayType { Show, Log };
  DisplayType display_type = DisplayType::Show;

  std::string method();
  Out_ShowLogMessageParams params;
};
template <typename TVisitor>
void Reflect(TVisitor& visitor, Out_ShowLogMessage& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(jsonrpc);
  std::string method = value.method();
  REFLECT_MEMBER2("method", method);
  REFLECT_MEMBER(params);
  REFLECT_MEMBER_END();
}

struct Out_Progress : public lsOutMessage<Out_Progress> {
  struct Params {
    int indexRequestCount = 0;
    int doIdMapCount = 0;
    int loadPreviousIndexCount = 0;
    int onIdMappedCount = 0;
    int onIndexedCount = 0;
    int activeThreads = 0;
  };
  std::string method = "$cquery/progress";
  Params params;
};
MAKE_REFLECT_STRUCT(Out_Progress::Params,
                    indexRequestCount,
                    doIdMapCount,
                    loadPreviousIndexCount,
                    onIdMappedCount,
                    onIndexedCount,
                    activeThreads);
MAKE_REFLECT_STRUCT(Out_Progress, jsonrpc, method, params);

struct Out_CquerySetInactiveRegion
    : public lsOutMessage<Out_CquerySetInactiveRegion> {
  struct Params {
    lsDocumentUri uri;
    std::vector<lsRange> inactiveRegions;
  };
  std::string method = "$cquery/setInactiveRegions";
  Params params;
};
MAKE_REFLECT_STRUCT(Out_CquerySetInactiveRegion::Params, uri, inactiveRegions);
MAKE_REFLECT_STRUCT(Out_CquerySetInactiveRegion, jsonrpc, method, params);

struct Out_LocationList : public lsOutMessage<Out_LocationList> {
  lsRequestId id;
  std::vector<lsLocation> result;
};
MAKE_REFLECT_STRUCT(Out_LocationList, jsonrpc, id, result);
