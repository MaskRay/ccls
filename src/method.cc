#include "method.h"

MethodType kMethodType_Unknown = "$unknown";
MethodType kMethodType_Exit = "exit";
MethodType kMethodType_TextDocumentPublishDiagnostics = "textDocument/publishDiagnostics";
MethodType kMethodType_CqueryPublishInactiveRegions = "$cquery/publishInactiveRegions";
MethodType kMethodType_CqueryPublishSemanticHighlighting = "$cquery/publishSemanticHighlighting";

InMessage::~InMessage() = default;

lsRequestId RequestInMessage::GetRequestId() const {
  return id;
}

lsRequestId NotificationInMessage::GetRequestId() const {
  return std::monostate();
}