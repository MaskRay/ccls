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

#include "config.hh"
#include "serializer.hh"
#include "utils.hh"

#include <rapidjson/fwd.h>

#include <chrono>
#include <iosfwd>

namespace ccls {
struct RequestId {
  // The client can send the request id as an int or a string. We should output
  // the same format we received.
  enum Type { kNone, kInt, kString };
  Type type = kNone;

  int value = -1;

  bool Valid() const { return type != kNone; }
};
void Reflect(JsonReader &visitor, RequestId &value);
void Reflect(JsonWriter &visitor, RequestId &value);

struct InMessage {
  RequestId id;
  std::string method;
  std::unique_ptr<char[]> message;
  std::unique_ptr<rapidjson::Document> document;
  std::chrono::steady_clock::time_point deadline;
  std::string backlog_path;
};

enum class ErrorCode {
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

struct ResponseError {
  ErrorCode code;
  std::string message;
};

constexpr char ccls_xref[] = "ccls.xref";
constexpr char window_showMessage[] = "window/showMessage";

struct DocumentUri {
  static DocumentUri FromPath(const std::string &path);

  bool operator==(const DocumentUri &o) const { return raw_uri == o.raw_uri; }
  bool operator<(const DocumentUri &o) const { return raw_uri < o.raw_uri; }

  void SetPath(const std::string &path);
  std::string GetPath() const;

  std::string raw_uri;
};

struct Position {
  int line = 0;
  int character = 0;
  bool operator==(const Position &o) const {
    return line == o.line && character == o.character;
  }
  bool operator<(const Position &o) const {
    return line != o.line ? line < o.line : character < o.character;
  }
  bool operator<=(const Position &o) const {
    return line != o.line ? line < o.line : character <= o.character;
  }
  std::string ToString() const;
};

struct lsRange {
  Position start;
  Position end;
  bool operator==(const lsRange &o) const {
    return start == o.start && end == o.end;
  }
  bool operator<(const lsRange &o) const {
    return !(start == o.start) ? start < o.start : end < o.end;
  }
  bool Includes(const lsRange &o) const {
    return start <= o.start && o.end <= end;
  }
  bool Intersects(const lsRange &o) const {
    return start < o.end && o.start < end;
  }
};

struct Location {
  DocumentUri uri;
  lsRange range;
  bool operator==(const Location &o) const {
    return uri == o.uri && range == o.range;
  }
  bool operator<(const Location &o) const {
    return !(uri == o.uri) ? uri < o.uri : range < o.range;
  }
};

struct LocationLink {
  std::string targetUri;
  lsRange targetRange;
  lsRange targetSelectionRange;
  explicit operator bool() const { return targetUri.size(); }
  explicit operator Location() && {
    return {DocumentUri{std::move(targetUri)}, targetSelectionRange};
  }
  bool operator==(const LocationLink &o) const {
    return targetUri == o.targetUri &&
           targetSelectionRange == o.targetSelectionRange;
  }
  bool operator<(const LocationLink &o) const {
    return !(targetUri == o.targetUri)
               ? targetUri < o.targetUri
               : targetSelectionRange < o.targetSelectionRange;
  }
};

enum class SymbolKind : uint8_t {
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

struct SymbolInformation {
  std::string_view name;
  SymbolKind kind;
  Location location;
  std::optional<std::string_view> containerName;
};

struct TextDocumentIdentifier {
  DocumentUri uri;
};

struct VersionedTextDocumentIdentifier {
  DocumentUri uri;
  // The version number of this document.  number | null
  std::optional<int> version;
};

struct TextEdit {
  lsRange range;
  std::string newText;
};

struct TextDocumentItem {
  DocumentUri uri;
  std::string languageId;
  int version;
  std::string text;
};

struct TextDocumentContentChangeEvent {
  // The range of the document that changed.
  std::optional<lsRange> range;
  // The length of the range that got replaced.
  std::optional<int> rangeLength;
  // The new text of the range/document.
  std::string text;
};

struct TextDocumentDidChangeParam {
  VersionedTextDocumentIdentifier textDocument;
  std::vector<TextDocumentContentChangeEvent> contentChanges;
};

struct WorkspaceFolder {
  DocumentUri uri;
  std::string name;
};

enum class MessageType : int { Error = 1, Warning = 2, Info = 3, Log = 4 };
REFLECT_UNDERLYING(MessageType)

struct Diagnostic {
  lsRange range;
  int severity = 0;
  int code = 0;
  std::string source = "ccls";
  std::string message;
  std::vector<TextEdit> fixits_;
};

struct ShowMessageParam {
  MessageType type = MessageType::Error;
  std::string message;
};

// Used to identify the language at a file level. The ordering is important, as
// a file previously identified as `C`, will be changed to `Cpp` if it
// encounters a c++ declaration.
enum class LanguageId { Unknown = -1, C = 0, Cpp = 1, ObjC = 2, ObjCpp = 3 };

} // namespace ccls
