#pragma once

#include "ipc.h"
#include "serializer.h"
#include "utils.h"

#include <optional.h>
#include <rapidjson/writer.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

using std::experimental::optional;
using std::experimental::nullopt;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////////////////// OUTGOING MESSAGES /////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct lsRequestId {
  optional<int> id0;
  optional<std::string> id1;
};
void Reflect(Writer& visitor, lsRequestId& value);
void Reflect(Reader& visitor, lsRequestId& id);

























struct IndexerConfig {
  // Root directory of the project. **Not serialized**
  std::string projectRoot;

  std::string cacheDirectory;
  NonElidedVector<std::string> whitelist;
  NonElidedVector<std::string> blacklist;
  std::vector<std::string> extraClangArguments;

  // Maximum workspace search results.
  int maxWorkspaceSearchResults = 1000;

  // Force a certain number of indexer threads. If less than 1 a default value
  // should be used.
  int indexerCount = 0;
  // If false, the indexer will be disabled.
  bool enableIndexing = true;
  // If false, indexed files will not be written to disk.
  bool enableCacheWrite = true;
  // If false, the index will not be loaded from a previous run.
  bool enableCacheRead = true;

  // If true, document links are reported for #include directives.
  bool showDocumentLinksOnIncludes = true;

  // Enables code lens on parameter and function variables.
  bool codeLensOnLocalVariables = true;

  // Version of the client.
  int clientVersion = 0;
};
MAKE_REFLECT_STRUCT(IndexerConfig,
  cacheDirectory,
  whitelist, blacklist,
  extraClangArguments,

  maxWorkspaceSearchResults,
  indexerCount,
  enableIndexing, enableCacheWrite, enableCacheRead,

  showDocumentLinksOnIncludes,

  codeLensOnLocalVariables,

  clientVersion);

















/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
///////////////////////////// INCOMING MESSAGES /////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct MessageRegistry {
  static MessageRegistry* instance_;
  static MessageRegistry* instance();

  using Allocator = std::function<std::unique_ptr<BaseIpcMessage>(Reader& visitor)>;
  std::unordered_map<std::string, Allocator> allocators;

  template<typename T>
  void Register() {
    std::string method_name = IpcIdToString(T::kIpcId);
    allocators[method_name] = [](Reader& visitor) {
      auto result = MakeUnique<T>();
      Reflect(visitor, *result);
      return result;
    };
  }

  std::unique_ptr<BaseIpcMessage> ReadMessageFromStdin();
  std::unique_ptr<BaseIpcMessage> Parse(Reader& visitor);
};


struct lsBaseOutMessage {
  virtual void Write(std::ostream& out) = 0;
};

template<typename TDerived>
struct lsOutMessage : lsBaseOutMessage {
  // All derived types need to reflect on the |jsonrpc| member.
  std::string jsonrpc = "2.0";

  // Send the message to the language client by writing it to stdout.
  void Write(std::ostream& out) override {
    rapidjson::StringBuffer output;
    Writer writer(output);
    auto that = static_cast<TDerived*>(this);
    Reflect(writer, *that);

    out << "Content-Length: " << output.GetSize();
    out << (char)13 << char(10) << char(13) << char(10); // CRLFCRLF
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

template<typename TVisitor>
void Reflect(TVisitor& visitor, lsDocumentUri& value) {
  Reflect(visitor, value.raw_uri);
}


struct lsPosition {
  lsPosition();
  lsPosition(int line, int character);

  bool operator==(const lsPosition& other) const;

  std::string ToString() const;

  // Note: these are 0-based.
  int line = 0;
  int character = 0;
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
MAKE_REFLECT_TYPE_PROXY(lsSymbolKind, int);

struct lsSymbolInformation {
  std::string name;
  lsSymbolKind kind;
  lsLocation location;
  std::string containerName;
};
MAKE_REFLECT_STRUCT(lsSymbolInformation, name, kind, location, containerName);

template<typename T>
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
template<typename TVisitor, typename T>
void Reflect(TVisitor& visitor, lsCommand<T>& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(title);
  REFLECT_MEMBER(command);
  REFLECT_MEMBER(arguments);
  REFLECT_MEMBER_END();
}

template<typename TData, typename TCommandArguments>
struct lsCodeLens {
  // The range in which this code lens is valid. Should only span a single line.
  lsRange range;
  // The command this code lens represents.
  optional<lsCommand<TCommandArguments>> command;
  // A data entry field that is preserved on a code lens item between
  // a code lens and a code lens resolve request.
  TData data;
};
template<typename TVisitor, typename TData, typename TCommandArguments>
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
  // The version number of this document.
  int version = 0;
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
  // See also: https://github.com/Microsoft/vscode/blob/master/src/vs/editor/contrib/snippet/common/snippet.md
  Snippet = 2
};
MAKE_REFLECT_TYPE_PROXY(lsInsertTextFormat, int);

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
  Reference = 18
};
MAKE_REFLECT_TYPE_PROXY(lsCompletionItemKind, int);

struct lsCompletionItem {
  // A set of function parameters. Used internally for signature help. Not sent to vscode.
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

  // A string that shoud be used when comparing this item
  // with other items. When `falsy` the label is used.
  std::string sortText;

  // A string that should be used when filtering a set of
  // completion items. When `falsy` the label is used.
  //std::string filterText;

  // A string that should be inserted a document when selecting
  // this completion. When `falsy` the label is used.
  std::string insertText;

  // The format of the insert text. The format applies to both the `insertText` property
  // and the `newText` property of a provided `textEdit`.
  lsInsertTextFormat insertTextFormat = lsInsertTextFormat::Snippet;

  // An edit which is applied to a document when selecting this completion. When an edit is provided the value of
  // `insertText` is ignored.
  //
  // *Note:* The range of the edit must be a single line range and it must contain the position at which completion
  // has been requested.
  optional<lsTextEdit> textEdit;

  // An optional array of additional text edits that are applied when
  // selecting this completion. Edits must not overlap with the main edit
  // nor with themselves.
  // std::vector<TextEdit> additionalTextEdits;

  // An optional command that is executed *after* inserting this completion. *Note* that
  // additional modifications to the current document should be described with the
  // additionalTextEdits-property.
  // Command command;

  // An data entry field that is preserved on a completion item between
  // a completion and a completion resolve request.
  // data ? : any
};
MAKE_REFLECT_STRUCT(lsCompletionItem,
  label,
  kind,
  detail,
  documentation,
  sortText,
  insertText,
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
  //std::unordered_map<lsDocumentUri, std::vector<lsTextEdit>> changes;

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
MAKE_REFLECT_TYPE_PROXY(lsDocumentHighlightKind, int);

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
MAKE_REFLECT_TYPE_PROXY(lsDiagnosticSeverity, int);

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
  NonElidedVector<lsTextEdit> fixits_;
};
MAKE_REFLECT_STRUCT(lsDiagnostic, range, severity, source, message);


// TODO: DocumentFilter
// TODO: DocumentSelector



























































































/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////////////////////////// INITIALIZATION ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Workspace specific client capabilities.
struct lsWorkspaceClientCapabilites {
  // The client supports applying batch edits to the workspace.
  optional<bool> applyEdit;

  struct lsWorkspaceEdit {
    // The client supports versioned document changes in `WorkspaceEdit`s
    optional<bool> documentChanges;
  };

  // Capabilities specific to `WorkspaceEdit`s
  optional<lsWorkspaceEdit> workspaceEdit;


  struct lsGenericDynamicReg {
    // Did foo notification supports dynamic registration.
    optional<bool> dynamicRegistration;
  };


  // Capabilities specific to the `workspace/didChangeConfiguration` notification.
  optional<lsGenericDynamicReg> didChangeConfiguration;

  // Capabilities specific to the `workspace/didChangeWatchedFiles` notification.
  optional<lsGenericDynamicReg> didChangeWatchedFiles;

  // Capabilities specific to the `workspace/symbol` request.
  optional<lsGenericDynamicReg> symbol;

  // Capabilities specific to the `workspace/executeCommand` request.
  optional<lsGenericDynamicReg> executeCommand;
};

MAKE_REFLECT_STRUCT(lsWorkspaceClientCapabilites::lsWorkspaceEdit, documentChanges);
MAKE_REFLECT_STRUCT(lsWorkspaceClientCapabilites::lsGenericDynamicReg, dynamicRegistration);
MAKE_REFLECT_STRUCT(lsWorkspaceClientCapabilites,
  applyEdit,
  workspaceEdit,
  didChangeConfiguration,
  didChangeWatchedFiles,
  symbol,
  executeCommand);



// Text document specific client capabilities.
struct lsTextDocumentClientCapabilities {
  struct lsSynchronization {
    // Whether text document synchronization supports dynamic registration.
    optional<bool> dynamicRegistration;

    // The client supports sending will save notifications.
    optional<bool> willSave;

    // The client supports sending a will save request and
    // waits for a response providing text edits which will
    // be applied to the document before it is saved.
    optional<bool> willSaveWaitUntil;

    // The client supports did save notifications.
    optional<bool> didSave;
  };

  lsSynchronization synchronization;

  struct lsCompletion {
    // Whether completion supports dynamic registration.
    optional<bool> dynamicRegistration;

    struct lsCompletionItem {
      // Client supports snippets as insert text.
      //
      // A snippet can define tab stops and placeholders with `$1`, `$2`
      // and `${3:foo}`. `$0` defines the final tab stop, it defaults to
      // the end of the snippet. Placeholders with equal identifiers are linked,
      // that is typing in one will update others too.
      optional<bool> snippetSupport;
    };

    // The client supports the following `CompletionItem` specific
    // capabilities.
    optional<lsCompletionItem> completionItem;
  };
  // Capabilities specific to the `textDocument/completion`
  optional<lsCompletion> completion;

  struct lsGenericDynamicReg {
    // Whether foo supports dynamic registration.
    optional<bool> dynamicRegistration;
  };

  // Capabilities specific to the `textDocument/hover`
  optional<lsGenericDynamicReg> hover;

  // Capabilities specific to the `textDocument/signatureHelp`
  optional<lsGenericDynamicReg> signatureHelp;

  // Capabilities specific to the `textDocument/references`
  optional<lsGenericDynamicReg> references;

  // Capabilities specific to the `textDocument/documentHighlight`
  optional<lsGenericDynamicReg> documentHighlight;

  // Capabilities specific to the `textDocument/documentSymbol`
  optional<lsGenericDynamicReg> documentSymbol;

  // Capabilities specific to the `textDocument/formatting`
  optional<lsGenericDynamicReg> formatting;

  // Capabilities specific to the `textDocument/rangeFormatting`
  optional<lsGenericDynamicReg> rangeFormatting;

  // Capabilities specific to the `textDocument/onTypeFormatting`
  optional<lsGenericDynamicReg> onTypeFormatting;

  // Capabilities specific to the `textDocument/definition`
  optional<lsGenericDynamicReg> definition;

  // Capabilities specific to the `textDocument/codeAction`
  optional<lsGenericDynamicReg> codeAction;

  struct CodeLensRegistrationOptions : public lsGenericDynamicReg {
    // Code lens has a resolve provider as well.
    bool resolveProvider;
  };

  // Capabilities specific to the `textDocument/codeLens`
  optional<CodeLensRegistrationOptions> codeLens;

  // Capabilities specific to the `textDocument/documentLink`
  optional<lsGenericDynamicReg> documentLink;

  // Capabilities specific to the `textDocument/rename`
  optional<lsGenericDynamicReg> rename;
};

MAKE_REFLECT_STRUCT(lsTextDocumentClientCapabilities::lsSynchronization, dynamicRegistration, willSave, willSaveWaitUntil, didSave);
MAKE_REFLECT_STRUCT(lsTextDocumentClientCapabilities::lsCompletion, dynamicRegistration, completionItem);
MAKE_REFLECT_STRUCT(lsTextDocumentClientCapabilities::lsCompletion::lsCompletionItem, snippetSupport);
MAKE_REFLECT_STRUCT(lsTextDocumentClientCapabilities::lsGenericDynamicReg, dynamicRegistration);
MAKE_REFLECT_STRUCT(lsTextDocumentClientCapabilities::CodeLensRegistrationOptions, dynamicRegistration, resolveProvider);
MAKE_REFLECT_STRUCT(lsTextDocumentClientCapabilities,
  synchronization,
  completion,
  hover,
  signatureHelp,
  references,
  documentHighlight,
  documentSymbol,
  formatting,
  rangeFormatting,
  onTypeFormatting,
  definition,
  codeAction,
  codeLens,
  documentLink,
  rename);

struct lsClientCapabilities {
  // Workspace specific client capabilities.
  optional<lsWorkspaceClientCapabilites> workspace;

  // Text document specific client capabilities.
  optional<lsTextDocumentClientCapabilities> textDocument;

  /**
  * Experimental client capabilities.
  */
  // experimental?: any; // TODO
};
MAKE_REFLECT_STRUCT(lsClientCapabilities, workspace, textDocument);

struct lsInitializeParams {
  // The process Id of the parent process that started
  // the server. Is null if the process has not been started by another process.
  // If the parent process is not alive then the server should exit (see exit notification) its process.
  optional<int> processId;

  // The rootPath of the workspace. Is null
  // if no folder is open.
  //
  // @deprecated in favour of rootUri.
  optional<std::string> rootPath;

  // The rootUri of the workspace. Is null if no
  // folder is open. If both `rootPath` and `rootUri` are set
  // `rootUri` wins.
  optional<lsDocumentUri> rootUri;

  // User provided initialization options.
  optional<IndexerConfig> initializationOptions;

  // The capabilities provided by the client (editor or tool)
  lsClientCapabilities capabilities;

  enum class lsTrace {
    // NOTE: serialized as a string, one of 'off' | 'messages' | 'verbose';
    Off, // off
    Messages, // messages
    Verbose // verbose
  };

  // The initial trace setting. If omitted trace is disabled ('off').
  lsTrace trace = lsTrace::Off;
};
void Reflect(Reader& reader, lsInitializeParams::lsTrace& value);
void Reflect(Writer& writer, lsInitializeParams::lsTrace& value);
MAKE_REFLECT_STRUCT(lsInitializeParams, processId, rootPath, rootUri, initializationOptions, capabilities, trace);


struct lsInitializeError {
  // Indicates whether the client should retry to send the
  // initilize request after showing the message provided
  // in the ResponseError.
  bool retry;
};
MAKE_REFLECT_STRUCT(lsInitializeError, retry);

// Defines how the host (editor) should sync document changes to the language server.
enum class lsTextDocumentSyncKind {
  // Documents should not be synced at all.
  None = 0,

  // Documents are synced by always sending the full content
  // of the document.
  Full = 1,

  // Documents are synced by sending the full content on open.
  // After that only incremental updates to the document are
  // send.
  Incremental = 2
};
MAKE_REFLECT_TYPE_PROXY(lsTextDocumentSyncKind, int)

// Completion options.
struct lsCompletionOptions {
  // The server provides support to resolve additional
  // information for a completion item.
  bool resolveProvider = false;

  // The characters that trigger completion automatically.
  std::vector<std::string> triggerCharacters;
};
MAKE_REFLECT_STRUCT(lsCompletionOptions, resolveProvider, triggerCharacters);

// Signature help options.
struct lsSignatureHelpOptions {
  // The characters that trigger signature help automatically.
  std::vector<std::string> triggerCharacters;
};
MAKE_REFLECT_STRUCT(lsSignatureHelpOptions, triggerCharacters);

// Code Lens options.
struct lsCodeLensOptions {
  // Code lens has a resolve provider as well.
  bool resolveProvider = false;
};
MAKE_REFLECT_STRUCT(lsCodeLensOptions, resolveProvider);

// Format document on type options
struct lsDocumentOnTypeFormattingOptions {
  // A character on which formatting should be triggered, like `}`.
  std::string firstTriggerCharacter;

  // More trigger characters.
  NonElidedVector<std::string> moreTriggerCharacter;
};
MAKE_REFLECT_STRUCT(lsDocumentOnTypeFormattingOptions, firstTriggerCharacter, moreTriggerCharacter);

// Document link options
struct lsDocumentLinkOptions {
  // Document links have a resolve provider as well.
  bool resolveProvider = false;
};
MAKE_REFLECT_STRUCT(lsDocumentLinkOptions, resolveProvider);

// Execute command options.
struct lsExecuteCommandOptions {
  // The commands to be executed on the server
  NonElidedVector<std::string> commands;
};
MAKE_REFLECT_STRUCT(lsExecuteCommandOptions, commands);

// Save options.
struct lsSaveOptions {
  // The client is supposed to include the content on save.
  bool includeText = false;
};
MAKE_REFLECT_STRUCT(lsSaveOptions, includeText);

struct lsTextDocumentSyncOptions {
  // Open and close notifications are sent to the server.
  bool openClose = false;
  // Change notificatins are sent to the server. See TextDocumentSyncKind.None, TextDocumentSyncKind.Full
  // and TextDocumentSyncKindIncremental.
  lsTextDocumentSyncKind change = lsTextDocumentSyncKind::Incremental;
  // Will save notifications are sent to the server.
  optional<bool> willSave;
  // Will save wait until requests are sent to the server.
  optional<bool> willSaveWaitUntil;
  // Save notifications are sent to the server.
  optional<lsSaveOptions> save;
};
MAKE_REFLECT_STRUCT(lsTextDocumentSyncOptions, openClose, change, willSave, willSaveWaitUntil, save);

struct lsServerCapabilities {
  // Defines how text documents are synced. Is either a detailed structure defining each notification or
  // for backwards compatibility the TextDocumentSyncKind number.
  // TODO: It seems like the new API is broken and doesn't work.
  // optional<lsTextDocumentSyncOptions> textDocumentSync;
  lsTextDocumentSyncKind textDocumentSync;

  // The server provides hover support.
  bool hoverProvider = false;
  // The server provides completion support.
  optional<lsCompletionOptions> completionProvider;
  // The server provides signature help support.
  optional<lsSignatureHelpOptions> signatureHelpProvider;
  // The server provides goto definition support.
  bool definitionProvider = false;
  // The server provides find references support.
  bool referencesProvider = false;
  // The server provides document highlight support.
  bool documentHighlightProvider = false;
  // The server provides document symbol support.
  bool documentSymbolProvider = false;
  // The server provides workspace symbol support.
  bool workspaceSymbolProvider = false;
  // The server provides code actions.
  bool codeActionProvider = false;
  // The server provides code lens.
  optional<lsCodeLensOptions> codeLensProvider;
  // The server provides document formatting.
  bool documentFormattingProvider = false;
  // The server provides document range formatting.
  bool documentRangeFormattingProvider = false;
  // The server provides document formatting on typing.
  optional<lsDocumentOnTypeFormattingOptions> documentOnTypeFormattingProvider;
  // The server provides rename support.
  bool renameProvider = false;
  // The server provides document link support.
  optional<lsDocumentLinkOptions> documentLinkProvider;
  // The server provides execute command support.
  optional<lsExecuteCommandOptions> executeCommandProvider;
};
MAKE_REFLECT_STRUCT(lsServerCapabilities,
  textDocumentSync,
  hoverProvider,
  completionProvider,
  signatureHelpProvider,
  definitionProvider,
  referencesProvider,
  documentHighlightProvider,
  documentSymbolProvider,
  workspaceSymbolProvider,
  codeActionProvider,
  codeLensProvider,
  documentFormattingProvider,
  documentRangeFormattingProvider,
  documentOnTypeFormattingProvider,
  renameProvider,
  documentLinkProvider,
  executeCommandProvider);

struct Ipc_InitializeRequest : public IpcMessage<Ipc_InitializeRequest> {
  const static IpcId kIpcId = IpcId::Initialize;

  lsRequestId id;
  lsInitializeParams params;
};
MAKE_REFLECT_STRUCT(Ipc_InitializeRequest, id, params);

struct Out_InitializeResponse : public lsOutMessage<Out_InitializeResponse> {
  struct InitializeResult {
    lsServerCapabilities capabilities;
  };
  lsRequestId id;
  InitializeResult result;
};
MAKE_REFLECT_STRUCT(Out_InitializeResponse::InitializeResult, capabilities);
MAKE_REFLECT_STRUCT(Out_InitializeResponse, jsonrpc, id, result);

struct Ipc_InitializedNotification : public IpcMessage<Ipc_InitializedNotification> {
  const static IpcId kIpcId = IpcId::Initialized;

  lsRequestId id;
};
MAKE_REFLECT_STRUCT(Ipc_InitializedNotification, id);








struct Ipc_Exit : public IpcMessage<Ipc_Exit> {
  static const IpcId kIpcId = IpcId::Exit;
};
MAKE_REFLECT_EMPTY_STRUCT(Ipc_Exit);

































// Cancel an existing request.
struct Ipc_CancelRequest : public IpcMessage<Ipc_CancelRequest> {
  static const IpcId kIpcId = IpcId::CancelRequest;
  lsRequestId id;
};
MAKE_REFLECT_STRUCT(Ipc_CancelRequest, id);





// Open, update, close file
struct Ipc_TextDocumentDidOpen : public IpcMessage<Ipc_TextDocumentDidOpen> {
  struct Params {
    lsTextDocumentItem textDocument;
  };

  const static IpcId kIpcId = IpcId::TextDocumentDidOpen;
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidOpen::Params, textDocument);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidOpen, params);
struct Ipc_TextDocumentDidChange : public IpcMessage<Ipc_TextDocumentDidChange> {
  struct lsTextDocumentContentChangeEvent {
    // The range of the document that changed.
    lsRange range;
    // The length of the range that got replaced.
    int rangeLength = -1;
    // The new text of the range/document.
    std::string text;
  };

  struct Params {
    lsVersionedTextDocumentIdentifier textDocument;
    std::vector<lsTextDocumentContentChangeEvent> contentChanges;
  };

  const static IpcId kIpcId = IpcId::TextDocumentDidChange;
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidChange::lsTextDocumentContentChangeEvent, range, rangeLength, text);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidChange::Params, textDocument, contentChanges);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidChange, params);
struct Ipc_TextDocumentDidClose : public IpcMessage<Ipc_TextDocumentDidClose> {
  struct Params {
    lsTextDocumentItem textDocument;
  };

  const static IpcId kIpcId = IpcId::TextDocumentDidClose;
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidClose::Params, textDocument);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidClose, params);


struct Ipc_TextDocumentDidSave : public IpcMessage<Ipc_TextDocumentDidSave> {
  struct Params {
    // The document that was saved.
    lsTextDocumentIdentifier textDocument;

    // Optional the content when saved. Depends on the includeText value
    // when the save notifcation was requested.
    // std::string text;
  };

  const static IpcId kIpcId = IpcId::TextDocumentDidSave;
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidSave::Params, textDocument);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDidSave, params);



// Diagnostics
struct Out_TextDocumentPublishDiagnostics : public lsOutMessage<Out_TextDocumentPublishDiagnostics> {
  struct Params {
    // The URI for which diagnostic information is reported.
    lsDocumentUri uri;

    // An array of diagnostic information items.
    NonElidedVector<lsDiagnostic> diagnostics;
  };

  Params params;
};
template<typename TVisitor>
void Reflect(TVisitor& visitor, Out_TextDocumentPublishDiagnostics& value) {
  std::string method = "textDocument/publishDiagnostics";
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(jsonrpc);
  REFLECT_MEMBER2("method", method);
  REFLECT_MEMBER(params);
  REFLECT_MEMBER_END();
}
MAKE_REFLECT_STRUCT(Out_TextDocumentPublishDiagnostics::Params, uri, diagnostics);



// Rename
struct Ipc_TextDocumentRename : public IpcMessage<Ipc_TextDocumentRename> {
  struct Params {
    // The document to format.
    lsTextDocumentIdentifier textDocument;

    // The position at which this request was sent.
    lsPosition position;

    // The new name of the symbol. If the given name is not valid the
    // request must return a [ResponseError](#ResponseError) with an
    // appropriate message set.
    std::string newName;
  };
  const static IpcId kIpcId = IpcId::TextDocumentRename;

  lsRequestId id;
  Params params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentRename::Params, textDocument, position, newName);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentRename, id, params);
struct Out_TextDocumentRename : public lsOutMessage<Out_TextDocumentRename> {
  lsRequestId id;
  lsWorkspaceEdit result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentRename, jsonrpc, id, result);





// Code completion
struct Ipc_TextDocumentComplete : public IpcMessage<Ipc_TextDocumentComplete> {
  const static IpcId kIpcId = IpcId::TextDocumentCompletion;

  lsRequestId id;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentComplete, id, params);
struct lsTextDocumentCompleteResult {
  // This list it not complete. Further typing should result in recomputing
  // this list.
  bool isIncomplete = false;
  // The completion items.
  NonElidedVector<lsCompletionItem> items;
};
MAKE_REFLECT_STRUCT(lsTextDocumentCompleteResult, isIncomplete, items);
struct Out_TextDocumentComplete : public lsOutMessage<Out_TextDocumentComplete> {
  lsRequestId id;
  lsTextDocumentCompleteResult result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentComplete, jsonrpc, id, result);

// Signature help.
struct Ipc_TextDocumentSignatureHelp : public IpcMessage<Ipc_TextDocumentSignatureHelp> {
  const static IpcId kIpcId = IpcId::TextDocumentSignatureHelp;

  lsRequestId id;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentSignatureHelp, id, params);
// Represents a parameter of a callable-signature. A parameter can
// have a label and a doc-comment.
struct lsParameterInformation {
  // The label of this parameter. Will be shown in
  // the UI.
  std::string label;

  // The human-readable doc-comment of this parameter. Will be shown
  // in the UI but can be omitted.
  optional<std::string> documentation;
};
MAKE_REFLECT_STRUCT(lsParameterInformation, label, documentation);
// Represents the signature of something callable. A signature
// can have a label, like a function-name, a doc-comment, and
// a set of parameters.
struct lsSignatureInformation {
  // The label of this signature. Will be shown in
  // the UI.
  std::string label;

  // The human-readable doc-comment of this signature. Will be shown
  // in the UI but can be omitted.
  optional<std::string> documentation;

  // The parameters of this signature.
  std::vector<lsParameterInformation> parameters;
};
MAKE_REFLECT_STRUCT(lsSignatureInformation, label, documentation, parameters);
// Signature help represents the signature of something
// callable. There can be multiple signature but only one
// active and only one active parameter.
struct lsSignatureHelp {
  // One or more signatures.
  NonElidedVector<lsSignatureInformation> signatures;

  // The active signature. If omitted or the value lies outside the
  // range of `signatures` the value defaults to zero or is ignored if
  // `signatures.length === 0`. Whenever possible implementors should
  // make an active decision about the active signature and shouldn't
  // rely on a default value.
  // In future version of the protocol this property might become
  // mandantory to better express this.
  optional<int> activeSignature;

  // The active parameter of the active signature. If omitted or the value
  // lies outside the range of `signatures[activeSignature].parameters`
  // defaults to 0 if the active signature has parameters. If
  // the active signature has no parameters it is ignored.
  // In future version of the protocol this property might become
  // mandantory to better express the active parameter if the
  // active signature does have any.
  optional<int> activeParameter;
};
MAKE_REFLECT_STRUCT(lsSignatureHelp, signatures, activeSignature, activeParameter);
struct Out_TextDocumentSignatureHelp : public lsOutMessage<Out_TextDocumentSignatureHelp> {
  lsRequestId id;
  lsSignatureHelp result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentSignatureHelp, jsonrpc, id, result);

// Goto definition
struct Ipc_TextDocumentDefinition : public IpcMessage<Ipc_TextDocumentDefinition> {
  const static IpcId kIpcId = IpcId::TextDocumentDefinition;

  lsRequestId id;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDefinition, id, params);
struct Out_TextDocumentDefinition : public lsOutMessage<Out_TextDocumentDefinition> {
  lsRequestId id;
  NonElidedVector<lsLocation> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentDefinition, jsonrpc, id, result);

// Document highlight
struct Ipc_TextDocumentDocumentHighlight : public IpcMessage<Ipc_TextDocumentDocumentHighlight> {
  const static IpcId kIpcId = IpcId::TextDocumentDocumentHighlight;

  lsRequestId id;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDocumentHighlight, id, params);
struct Out_TextDocumentDocumentHighlight : public lsOutMessage<Out_TextDocumentDocumentHighlight> {
  lsRequestId id;
  NonElidedVector<lsDocumentHighlight> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentDocumentHighlight, jsonrpc, id, result);

// Hover
struct Ipc_TextDocumentHover : public IpcMessage<Ipc_TextDocumentHover> {
  const static IpcId kIpcId = IpcId::TextDocumentHover;

  lsRequestId id;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentHover, id, params);
struct Out_TextDocumentHover : public lsOutMessage<Out_TextDocumentHover> {
  struct Result {
    std::string contents;
    optional<lsRange> range;
  };

  lsRequestId id;
  Result result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentHover::Result, contents, range);
MAKE_REFLECT_STRUCT(Out_TextDocumentHover, jsonrpc, id, result);

// References
struct Ipc_TextDocumentReferences : public IpcMessage<Ipc_TextDocumentReferences> {
  struct lsReferenceContext {
    // Include the declaration of the current symbol.
    bool includeDeclaration;
  };
  struct lsReferenceParams : public lsTextDocumentPositionParams {
    lsTextDocumentIdentifier textDocument;
    lsPosition position;
    lsReferenceContext context;
  };

  const static IpcId kIpcId = IpcId::TextDocumentReferences;

  lsRequestId id;
  lsReferenceParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentReferences::lsReferenceContext, includeDeclaration);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentReferences::lsReferenceParams, textDocument, position, context);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentReferences, id, params);
struct Out_TextDocumentReferences : public lsOutMessage<Out_TextDocumentReferences> {
  lsRequestId id;
  NonElidedVector<lsLocation> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentReferences, jsonrpc, id, result);

// Code action
struct Ipc_TextDocumentCodeAction : public IpcMessage<Ipc_TextDocumentCodeAction> {
  const static IpcId kIpcId = IpcId::TextDocumentCodeAction;
  // Contains additional diagnostic information about the context in which
  // a code action is run.
  struct lsCodeActionContext {
	  // An array of diagnostics.
	  NonElidedVector<lsDiagnostic> diagnostics;
  };
  // Params for the CodeActionRequest
  struct lsCodeActionParams {
	  // The document in which the command was invoked.
	  lsTextDocumentIdentifier textDocument;
	  // The range for which the command was invoked.
    lsRange range;
	  // Context carrying additional information.
    lsCodeActionContext context;
  };

  lsRequestId id;
  lsCodeActionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentCodeAction::lsCodeActionContext, diagnostics);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentCodeAction::lsCodeActionParams, textDocument, range, context);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentCodeAction, id, params);
struct Out_TextDocumentCodeAction : public lsOutMessage<Out_TextDocumentCodeAction> {
  struct CommandArgs {
    lsDocumentUri textDocumentUri;
    NonElidedVector<lsTextEdit> edits;
  };
  using Command = lsCommand<CommandArgs>;

  lsRequestId id;
  NonElidedVector<Command> result;
};
MAKE_REFLECT_STRUCT_WRITER_AS_ARRAY(Out_TextDocumentCodeAction::CommandArgs, textDocumentUri, edits);
MAKE_REFLECT_STRUCT(Out_TextDocumentCodeAction, jsonrpc, id, result);

// List symbols in a document.
struct lsDocumentSymbolParams {
  lsTextDocumentIdentifier textDocument;
};
MAKE_REFLECT_STRUCT(lsDocumentSymbolParams, textDocument);
struct Ipc_TextDocumentDocumentSymbol : public IpcMessage<Ipc_TextDocumentDocumentSymbol> {
  const static IpcId kIpcId = IpcId::TextDocumentDocumentSymbol;

  lsRequestId id;
  lsDocumentSymbolParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDocumentSymbol, id, params);
struct Out_TextDocumentDocumentSymbol : public lsOutMessage<Out_TextDocumentDocumentSymbol> {
  lsRequestId id;
  NonElidedVector<lsSymbolInformation> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentDocumentSymbol, jsonrpc, id, result);

// List links a document
struct Ipc_TextDocumentDocumentLink : public IpcMessage<Ipc_TextDocumentDocumentLink> {
  const static IpcId kIpcId = IpcId::TextDocumentDocumentLink;

  struct DocumentLinkParams {
    // The document to provide document links for.
    lsTextDocumentIdentifier textDocument;
  };

  lsRequestId id;
  DocumentLinkParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDocumentLink::DocumentLinkParams, textDocument);
MAKE_REFLECT_STRUCT(Ipc_TextDocumentDocumentLink, id, params);
// A document link is a range in a text document that links to an internal or external resource, like another
// text document or a web site.
struct lsDocumentLink {
	// The range this link applies to.
	lsRange range;
	// The uri this link points to. If missing a resolve request is sent later.
	optional<lsDocumentUri> target;
};
MAKE_REFLECT_STRUCT(lsDocumentLink, range, target);
struct Out_TextDocumentDocumentLink : public lsOutMessage<Out_TextDocumentDocumentLink> {
  lsRequestId id;
  NonElidedVector<lsDocumentLink> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentDocumentLink, jsonrpc, id, result);


// List code lens in a document.
struct lsDocumentCodeLensParams {
  lsTextDocumentIdentifier textDocument;
};
MAKE_REFLECT_STRUCT(lsDocumentCodeLensParams, textDocument);
struct lsCodeLensUserData {};
MAKE_REFLECT_EMPTY_STRUCT(lsCodeLensUserData);
struct lsCodeLensCommandArguments {
  lsDocumentUri uri;
  lsPosition position;
  NonElidedVector<lsLocation> locations;
};
void Reflect(Writer& visitor, lsCodeLensCommandArguments& value);
void Reflect(Reader& visitor, lsCodeLensCommandArguments& value);
using TCodeLens = lsCodeLens<lsCodeLensUserData, lsCodeLensCommandArguments>;
struct Ipc_TextDocumentCodeLens : public IpcMessage<Ipc_TextDocumentCodeLens> {
  const static IpcId kIpcId = IpcId::TextDocumentCodeLens;
  lsRequestId id;
  lsDocumentCodeLensParams params;
};
MAKE_REFLECT_STRUCT(Ipc_TextDocumentCodeLens, id, params);
struct Out_TextDocumentCodeLens : public lsOutMessage<Out_TextDocumentCodeLens> {
  lsRequestId id;
  NonElidedVector<lsCodeLens<lsCodeLensUserData, lsCodeLensCommandArguments>> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentCodeLens, jsonrpc, id, result);
struct Ipc_CodeLensResolve : public IpcMessage<Ipc_CodeLensResolve> {
  const static IpcId kIpcId = IpcId::CodeLensResolve;

  lsRequestId id;
  TCodeLens params;
};
MAKE_REFLECT_STRUCT(Ipc_CodeLensResolve, id, params);
struct Out_CodeLensResolve : public lsOutMessage<Out_CodeLensResolve> {
  lsRequestId id;
  TCodeLens result;
};
MAKE_REFLECT_STRUCT(Out_CodeLensResolve, jsonrpc, id, result);

// Search for symbols in the workspace.
struct lsWorkspaceSymbolParams {
  std::string query;
};
MAKE_REFLECT_STRUCT(lsWorkspaceSymbolParams, query);
struct Ipc_WorkspaceSymbol : public IpcMessage<Ipc_WorkspaceSymbol> {
  const static IpcId kIpcId = IpcId::WorkspaceSymbol;
  lsRequestId id;
  lsWorkspaceSymbolParams params;
};
MAKE_REFLECT_STRUCT(Ipc_WorkspaceSymbol, id, params);
struct Out_WorkspaceSymbol : public lsOutMessage<Out_WorkspaceSymbol> {
  lsRequestId id;
  NonElidedVector<lsSymbolInformation> result;
};
MAKE_REFLECT_STRUCT(Out_WorkspaceSymbol, jsonrpc, id, result);

// Show a message to the user.
enum class lsMessageType : int {
  Error = 1,
  Warning = 2,
  Info = 3,
  Log = 4
};
MAKE_REFLECT_TYPE_PROXY(lsMessageType, int)
struct Out_ShowLogMessageParams {
  lsMessageType type = lsMessageType::Error;
  std::string message;
};
MAKE_REFLECT_STRUCT(Out_ShowLogMessageParams, type, message);
struct Out_ShowLogMessage : public lsOutMessage<Out_ShowLogMessage> {
  enum class DisplayType {
    Show, Log
  };
  DisplayType display_type = DisplayType::Show;

  std::string method();
  Out_ShowLogMessageParams params;
};
template<typename TVisitor>
void Reflect(TVisitor& visitor, Out_ShowLogMessage& value) {
  REFLECT_MEMBER_START();
  REFLECT_MEMBER(jsonrpc);
  REFLECT_MEMBER2("method", value.method());
  REFLECT_MEMBER(params);
  REFLECT_MEMBER_END();
}


struct Out_CquerySetInactiveRegion : public lsOutMessage<Out_CquerySetInactiveRegion> {
  struct Params {
    lsDocumentUri uri;
    NonElidedVector<lsRange> inactiveRegions;
  };
  std::string method = "$cquery/setInactiveRegions";
  Params params;
};
MAKE_REFLECT_STRUCT(Out_CquerySetInactiveRegion::Params, uri, inactiveRegions);
MAKE_REFLECT_STRUCT(Out_CquerySetInactiveRegion, jsonrpc, method, params);


struct Ipc_CqueryFreshenIndex : public IpcMessage<Ipc_CqueryFreshenIndex> {
  const static IpcId kIpcId = IpcId::CqueryFreshenIndex;
  lsRequestId id;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryFreshenIndex, id);


// Vars, Callers, Derived, GotoParent
struct Ipc_CqueryVars : public IpcMessage<Ipc_CqueryVars> {
  const static IpcId kIpcId = IpcId::CqueryVars;
  lsRequestId id;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryVars, id, params);
struct Ipc_CqueryCallers : public IpcMessage<Ipc_CqueryCallers> {
  const static IpcId kIpcId = IpcId::CqueryCallers;
  lsRequestId id;
  lsTextDocumentPositionParams params;
};
struct Ipc_CqueryBase : public IpcMessage<Ipc_CqueryBase> {
  const static IpcId kIpcId = IpcId::CqueryBase;
  lsRequestId id;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryBase, id, params);
MAKE_REFLECT_STRUCT(Ipc_CqueryCallers, id, params);
struct Ipc_CqueryDerived : public IpcMessage<Ipc_CqueryDerived> {
  const static IpcId kIpcId = IpcId::CqueryDerived;
  lsRequestId id;
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(Ipc_CqueryDerived, id, params);
struct Out_LocationList : public lsOutMessage<Out_LocationList> {
  lsRequestId id;
  NonElidedVector<lsLocation> result;
};
MAKE_REFLECT_STRUCT(Out_LocationList, jsonrpc, id, result);