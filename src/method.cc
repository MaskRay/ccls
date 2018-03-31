#include "method.h"

MethodType kMethodType_Unknown = "$unknown";
MethodType kMethodType_Exit = "exit";
MethodType kMethodType_TextDocumentPublishDiagnostics = "textDocument/publishDiagnostics";
MethodType kMethodType_CclsPublishInactiveRegions = "$ccls/publishInactiveRegions";
MethodType kMethodType_CclsPublishSemanticHighlighting = "$ccls/publishSemanticHighlighting";

InMessage::~InMessage() = default;

lsRequestId RequestInMessage::GetRequestId() const {
  return id;
}

lsRequestId NotificationInMessage::GetRequestId() const {
  return std::monostate();
}