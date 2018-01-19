#pragma once

#include "clang_symbol_kind.h"
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
      bool log_stdin_to_stderr,
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
MAKE_REFLECT_TYPE_PROXY(lsSymbolKind, int);

struct lsSymbolInformation {
  std::string name;
  lsSymbolKind kind;
  lsLocation location;
  std::string containerName;
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
  Reference = 18,
  Folder = 19,
  EnumMember = 20,
  Constant = 21,
  Struct = 22,
  Event = 23,
  Operator = 24,
  TypeParameter = 25,
};
MAKE_REFLECT_TYPE_PROXY(lsCompletionItemKind, int);

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
  std::string::size_type pos_;
  unsigned priority_;

  // A string that shoud be used when comparing this item
  // with other items. When `falsy` the label is used.
  std::string sortText;

  // A string that should be used when filtering a set of
  // completion items. When `falsy` the label is used.
  // std::string filterText;

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
MAKE_REFLECT_TYPE_PROXY(lsDocumentHighlightKind, int);

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
  std::vector<lsTextEdit> fixits_;
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

  // Capabilities specific to the `workspace/didChangeConfiguration`
  // notification.
  optional<lsGenericDynamicReg> didChangeConfiguration;

  // Capabilities specific to the `workspace/didChangeWatchedFiles`
  // notification.
  optional<lsGenericDynamicReg> didChangeWatchedFiles;

  // Capabilities specific to the `workspace/symbol` request.
  optional<lsGenericDynamicReg> symbol;

  // Capabilities specific to the `workspace/executeCommand` request.
  optional<lsGenericDynamicReg> executeCommand;
};

MAKE_REFLECT_STRUCT(lsWorkspaceClientCapabilites::lsWorkspaceEdit,
                    documentChanges);
MAKE_REFLECT_STRUCT(lsWorkspaceClientCapabilites::lsGenericDynamicReg,
                    dynamicRegistration);
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

MAKE_REFLECT_STRUCT(lsTextDocumentClientCapabilities::lsSynchronization,
                    dynamicRegistration,
                    willSave,
                    willSaveWaitUntil,
                    didSave);
MAKE_REFLECT_STRUCT(lsTextDocumentClientCapabilities::lsCompletion,
                    dynamicRegistration,
                    completionItem);
MAKE_REFLECT_STRUCT(
    lsTextDocumentClientCapabilities::lsCompletion::lsCompletionItem,
    snippetSupport);
MAKE_REFLECT_STRUCT(lsTextDocumentClientCapabilities::lsGenericDynamicReg,
                    dynamicRegistration);
MAKE_REFLECT_STRUCT(
    lsTextDocumentClientCapabilities::CodeLensRegistrationOptions,
    dynamicRegistration,
    resolveProvider);
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
  // If the parent process is not alive then the server should exit (see exit
  // notification) its process.
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
  optional<Config> initializationOptions;

  // The capabilities provided by the client (editor or tool)
  lsClientCapabilities capabilities;

  enum class lsTrace {
    // NOTE: serialized as a string, one of 'off' | 'messages' | 'verbose';
    Off,       // off
    Messages,  // messages
    Verbose    // verbose
  };

  // The initial trace setting. If omitted trace is disabled ('off').
  lsTrace trace = lsTrace::Off;
};
void Reflect(Reader& reader, lsInitializeParams::lsTrace& value);
void Reflect(Writer& writer, lsInitializeParams::lsTrace& value);
MAKE_REFLECT_STRUCT(lsInitializeParams,
                    processId,
                    rootPath,
                    rootUri,
                    initializationOptions,
                    capabilities,
                    trace);

struct lsInitializeError {
  // Indicates whether the client should retry to send the
  // initilize request after showing the message provided
  // in the ResponseError.
  bool retry;
};
MAKE_REFLECT_STRUCT(lsInitializeError, retry);

// Defines how the host (editor) should sync document changes to the language
// server.
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
  std::vector<std::string> moreTriggerCharacter;
};
MAKE_REFLECT_STRUCT(lsDocumentOnTypeFormattingOptions,
                    firstTriggerCharacter,
                    moreTriggerCharacter);

// Document link options
struct lsDocumentLinkOptions {
  // Document links have a resolve provider as well.
  bool resolveProvider = false;
};
MAKE_REFLECT_STRUCT(lsDocumentLinkOptions, resolveProvider);

// Execute command options.
struct lsExecuteCommandOptions {
  // The commands to be executed on the server
  std::vector<std::string> commands;
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
  // Change notificatins are sent to the server. See TextDocumentSyncKind.None,
  // TextDocumentSyncKind.Full and TextDocumentSyncKindIncremental.
  lsTextDocumentSyncKind change = lsTextDocumentSyncKind::Incremental;
  // Will save notifications are sent to the server.
  optional<bool> willSave;
  // Will save wait until requests are sent to the server.
  optional<bool> willSaveWaitUntil;
  // Save notifications are sent to the server.
  optional<lsSaveOptions> save;
};
MAKE_REFLECT_STRUCT(lsTextDocumentSyncOptions,
                    openClose,
                    change,
                    willSave,
                    willSaveWaitUntil,
                    save);

struct lsServerCapabilities {
  // Defines how text documents are synced. Is either a detailed structure
  // defining each notification or for backwards compatibility the
  // TextDocumentSyncKind number.
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
MAKE_REFLECT_TYPE_PROXY(lsErrorCodes, int);
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
  std::string language;
  std::string value;
};
using lsMarkedString = std::variant<std::string, lsMarkedString1>;
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
MAKE_REFLECT_TYPE_PROXY(lsMessageType, int)
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

struct Out_CqueryPublishSemanticHighlighting
    : public lsOutMessage<Out_CqueryPublishSemanticHighlighting> {
  enum class SymbolType { Type = 0, Function, Variable };
  struct Symbol {
    int stableId = 0;
    // TODO Deprecate |type| in favor of fine-grained |kind|.
    SymbolType type = SymbolType::Type;
    ClangSymbolKind kind;
    bool isTypeMember = false;
    std::vector<lsRange> ranges;
  };
  struct Params {
    lsDocumentUri uri;
    std::vector<Symbol> symbols;
  };
  std::string method = "$cquery/publishSemanticHighlighting";
  Params params;
};
MAKE_REFLECT_TYPE_PROXY(Out_CqueryPublishSemanticHighlighting::SymbolType, int);
MAKE_REFLECT_STRUCT(Out_CqueryPublishSemanticHighlighting::Symbol,
                    type,
                    kind,
                    isTypeMember,
                    stableId,
                    ranges);
MAKE_REFLECT_STRUCT(Out_CqueryPublishSemanticHighlighting::Params,
                    uri,
                    symbols);
MAKE_REFLECT_STRUCT(Out_CqueryPublishSemanticHighlighting,
                    jsonrpc,
                    method,
                    params);

struct Out_LocationList : public lsOutMessage<Out_LocationList> {
  lsRequestId id;
  std::vector<lsLocation> result;
};
MAKE_REFLECT_STRUCT(Out_LocationList, jsonrpc, id, result);
