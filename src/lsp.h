#pragma once

#include "config.h"
#include "method.h"
#include "serializer.h"
#include "utils.h"

#include <iosfwd>
#include <unordered_map>

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

#define REGISTER_IN_MESSAGE(type) \
  static MessageRegistryRegister<type> type##message_handler_instance_;

struct MessageRegistry {
  static MessageRegistry* instance_;
  static MessageRegistry* instance();

  using Allocator =
      std::function<void(Reader& visitor, std::unique_ptr<InMessage>*)>;
  std::unordered_map<std::string, Allocator> allocators;

  optional<std::string> ReadMessageFromStdin(
      std::unique_ptr<InMessage>* message);
  optional<std::string> Parse(Reader& visitor,
                              std::unique_ptr<InMessage>* message);
};

template <typename T>
struct MessageRegistryRegister {
  MessageRegistryRegister() {
    T dummy;
    std::string method_name = dummy.GetMethodType();
    MessageRegistry::instance()->allocators[method_name] =
        [](Reader& visitor, std::unique_ptr<InMessage>* message) {
          *message = std::make_unique<T>();
          // Reflect may throw and *message will be partially deserialized.
          Reflect(visitor, static_cast<T&>(**message));
        };
  }
};

struct lsBaseOutMessage {
  virtual ~lsBaseOutMessage();
  virtual void ReflectWriter(Writer&) = 0;

  // Send the message to the language client by writing it to stdout.
  void Write(std::ostream& out);
};

template <typename TDerived>
struct lsOutMessage : lsBaseOutMessage {
  // All derived types need to reflect on the |jsonrpc| member.
  std::string jsonrpc = "2.0";

  void ReflectWriter(Writer& writer) override {
    Reflect(writer, static_cast<TDerived&>(*this));
  }
};

struct lsResponseError {
  enum class lsErrorCodes : int {
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    serverErrorStart = -32099,
    serverErrorEnd = -32000,
    ServerNotInitialized = -32002,
    UnknownErrorCode = -32001,
    RequestCancelled = -32800,
  };

  lsErrorCodes code;
  // Short description.
  std::string message;

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
  bool operator<(const lsRange& other) const;

  lsPosition start;
  lsPosition end;
};
MAKE_HASHABLE(lsRange, t.start, t.end);
MAKE_REFLECT_STRUCT(lsRange, start, end);

struct lsLocation {
  lsLocation();
  lsLocation(lsDocumentUri uri, lsRange range);

  bool operator==(const lsLocation& other) const;
  bool operator<(const lsLocation& o) const;

  lsDocumentUri uri;
  lsRange range;
};
MAKE_HASHABLE(lsLocation, t.uri, t.range);
MAKE_REFLECT_STRUCT(lsLocation, uri, range);

enum class lsSymbolKind : uint8_t {
  Unknown = 0,

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
  Array = 18,
  Object = 19,
  Key = 20,
  Null = 21,
  EnumMember = 22,
  Struct = 23,
  Event = 24,
  Operator = 25,

  // For C++, this is interpreted as "template parameter" (including
  // non-type template parameters).
  TypeParameter = 26,

  // cquery extensions
  // See also https://github.com/Microsoft/language-server-protocol/issues/344
  // for new SymbolKind clang/Index/IndexSymbol.h clang::index::SymbolKind
  TypeAlias = 252,
  Parameter = 253,
  StaticMethod = 254,
  Macro = 255,
};
MAKE_REFLECT_TYPE_PROXY(lsSymbolKind);

// cquery extension
struct lsLocationEx : lsLocation {
  optional<std::string_view> containerName;
  optional<lsSymbolKind> parentKind;
  // Avoid circular dependency on symbol.h
  optional<uint16_t> role;
};
MAKE_REFLECT_STRUCT(lsLocationEx, uri, range, containerName, parentKind, role);

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

struct lsFormattingOptions {
  // Size of a tab in spaces.
  int tabSize;
  // Prefer spaces over tabs.
  bool insertSpaces;
};
MAKE_REFLECT_STRUCT(lsFormattingOptions, tabSize, insertSpaces);

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

struct Out_LocationList : public lsOutMessage<Out_LocationList> {
  lsRequestId id;
  std::vector<lsLocationEx> result;
};
MAKE_REFLECT_STRUCT(Out_LocationList, jsonrpc, id, result);
