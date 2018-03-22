#include "method.h"

const char* kMethodType_Unknown = "$unknown";
const char* kMethodType_Exit = "exit";
const char* kMethodType_TextDocumentPublishDiagnostics = "textDocument/publishDiagnostics";
const char* kMethodType_CqueryPublishInactiveRegions = "$cquery/publishInactiveRegions";
const char* kMethodType_CqueryPublishSemanticHighlighting = "$cquery/publishSemanticHighlighting";

InMessage::~InMessage() = default;

lsRequestId RequestInMessage::GetRequestId() const {
  return id;
}

lsRequestId NotificationInMessage::GetRequestId() const {
  return std::monostate();
}