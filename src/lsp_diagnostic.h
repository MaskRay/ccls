#pragma once

#include "lsp.h"

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
