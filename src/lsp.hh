// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "config.hh"
#include "serializer.hh"
#include "utils.hh"

#include <rapidjson/fwd.h>

#include <iosfwd>
#include <unordered_map>

namespace ccls {
struct lsRequestId {
  // The client can send the request id as an int or a string. We should output
  // the same format we received.
  enum Type { kNone, kInt, kString };
  Type type = kNone;

  int value = -1;

  bool Valid() const { return type != kNone; }
};
void Reflect(Reader &visitor, lsRequestId &value);
void Reflect(Writer &visitor, lsRequestId &value);

struct InMessage {
  lsRequestId id;
  std::string method;
  std::unique_ptr<char[]> message;
  std::unique_ptr<rapidjson::Document> document;
};

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

struct lsResponseError {
  // A number indicating the error type that occurred.
  lsErrorCodes code;

  // A string providing a short description of the error.
  std::string message;

  // A Primitive or Structured value that contains additional
  // information about the error. Can be omitted.
  // std::optional<D> data;
};
MAKE_REFLECT_STRUCT(lsResponseError, code, message);

constexpr char ccls_xref[] = "ccls.xref";
constexpr char window_showMessage[] = "window/showMessage";

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
////////////////////////////// PRIMITIVE TYPES //////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

struct lsDocumentUri {
  static lsDocumentUri FromPath(const std::string &path);

  bool operator==(const lsDocumentUri &other) const;

  void SetPath(const std::string &path);
  std::string GetPath() const;

  std::string raw_uri;
};

template <typename TVisitor>
void Reflect(TVisitor &visitor, lsDocumentUri &value) {
  Reflect(visitor, value.raw_uri);
}

struct lsPosition {
  int line = 0;
  int character = 0;
  bool operator==(const lsPosition &o) const {
    return line == o.line && character == o.character;
  }
  bool operator<(const lsPosition &o) const {
    return line != o.line ? line < o.line : character < o.character;
  }
  std::string ToString() const;
};
MAKE_REFLECT_STRUCT(lsPosition, line, character);

struct lsRange {
  lsPosition start;
  lsPosition end;
  bool operator==(const lsRange &o) const {
    return start == o.start && end == o.end;
  }
  bool operator<(const lsRange &o) const {
    return !(start == o.start) ? start < o.start : end < o.end;
  }
};
MAKE_REFLECT_STRUCT(lsRange, start, end);

struct lsLocation {
  lsDocumentUri uri;
  lsRange range;
  bool operator==(const lsLocation &o) const {
    return uri == o.uri && range == o.range;
  }
  bool operator<(const lsLocation &o) const {
    return !(uri.raw_uri == o.uri.raw_uri) ? uri.raw_uri < o.uri.raw_uri
                                           : range < o.range;
  }
};
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

  // ccls extensions
  // See also https://github.com/Microsoft/language-server-protocol/issues/344
  // for new SymbolKind clang/Index/IndexSymbol.h clang::index::SymbolKind
  TypeAlias = 252,
  Parameter = 253,
  StaticMethod = 254,
  Macro = 255,
};
MAKE_REFLECT_TYPE_PROXY(lsSymbolKind);

struct lsSymbolInformation {
  std::string_view name;
  lsSymbolKind kind;
  lsLocation location;
  std::optional<std::string_view> containerName;
};

struct lsTextDocumentIdentifier {
  lsDocumentUri uri;
};
MAKE_REFLECT_STRUCT(lsTextDocumentIdentifier, uri);

struct lsVersionedTextDocumentIdentifier {
  lsDocumentUri uri;
  // The version number of this document.  number | null
  std::optional<int> version;

  lsTextDocumentIdentifier AsTextDocumentIdentifier() const;
};
MAKE_REFLECT_STRUCT(lsVersionedTextDocumentIdentifier, uri, version);

struct lsTextEdit {
  // The range of the text document to be manipulated. To insert
  // text into a document create a range where start === end.
  lsRange range;

  // The string to be inserted. For delete operations use an
  // empty string.
  std::string newText;

  bool operator==(const lsTextEdit &that);
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

struct TextDocumentContentChangeEvent {
  // The range of the document that changed.
  std::optional<lsRange> range;
  // The length of the range that got replaced.
  std::optional<int> rangeLength;
  // The new text of the range/document.
  std::string text;
};

struct TextDocumentDidChangeParam {
  lsVersionedTextDocumentIdentifier textDocument;
  std::vector<TextDocumentContentChangeEvent> contentChanges;
};

struct WorkspaceFolder {
  lsDocumentUri uri;
  std::string name;
};
MAKE_REFLECT_STRUCT(WorkspaceFolder, uri, name);

// Show a message to the user.
enum class MessageType : int { Error = 1, Warning = 2, Info = 3, Log = 4 };
MAKE_REFLECT_TYPE_PROXY(MessageType)

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
  std::optional<lsDiagnosticSeverity> severity;

  // The diagnostic's code. Can be omitted.
  int code = 0;

  // A human-readable string describing the source of this
  // diagnostic, e.g. 'typescript' or 'super lint'.
  std::string source = "ccls";

  // The diagnostic's message.
  std::string message;

  // Non-serialized set of fixits.
  std::vector<lsTextEdit> fixits_;
};
MAKE_REFLECT_STRUCT(lsDiagnostic, range, severity, source, message);

struct lsPublishDiagnosticsParams {
  // The URI for which diagnostic information is reported.
  lsDocumentUri uri;

  // An array of diagnostic information items.
  std::vector<lsDiagnostic> diagnostics;
};
MAKE_REFLECT_STRUCT(lsPublishDiagnosticsParams, uri, diagnostics);

struct lsShowMessageParams {
  MessageType type = MessageType::Error;
  std::string message;
};
MAKE_REFLECT_STRUCT(lsShowMessageParams, type, message);

// Used to identify the language at a file level. The ordering is important, as
// a file previously identified as `C`, will be changed to `Cpp` if it
// encounters a c++ declaration.
enum class LanguageId { Unknown = -1, C = 0, Cpp = 1, ObjC = 2, ObjCpp = 3 };
MAKE_REFLECT_TYPE_PROXY(LanguageId);

// The order matters. In FindSymbolsAtLocation, we want Var/Func ordered in
// front of others.
enum class SymbolKind : uint8_t { Invalid, File, Type, Func, Var };
MAKE_REFLECT_TYPE_PROXY(SymbolKind);

enum class Role : uint16_t {
  None = 0,
  Declaration = 1 << 0,
  Definition = 1 << 1,
  Reference = 1 << 2,
  Read = 1 << 3,
  Write = 1 << 4,
  Call = 1 << 5,
  Dynamic = 1 << 6,
  Address = 1 << 7,
  Implicit = 1 << 8,
  All = (1 << 9) - 1,
};
MAKE_REFLECT_TYPE_PROXY(Role);

inline uint16_t operator&(Role lhs, Role rhs) {
  return uint16_t(lhs) & uint16_t(rhs);
}

inline Role operator|(Role lhs, Role rhs) {
  return Role(uint16_t(lhs) | uint16_t(rhs));
}
} // namespace ccls
