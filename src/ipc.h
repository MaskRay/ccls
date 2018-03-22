#pragma once

#include "serializer.h"
#include "utils.h"

#include <string>

using lsRequestId = std::variant<std::monostate, int64_t, std::string>;

using MethodType = std::string;

extern const char* kMethodType_Unknown;
extern const char* kMethodType_Exit;
extern const char* kMethodType_TextDocumentPublishDiagnostics;
extern const char* kMethodType_CqueryPublishInactiveRegions;
extern const char* kMethodType_CqueryPublishSemanticHighlighting;

struct BaseIpcMessage {
  virtual ~BaseIpcMessage();

  virtual MethodType GetMethodType() const = 0;
  virtual lsRequestId GetRequestId();
};

struct RequestMessage : public BaseIpcMessage {
  // number or string, actually no null
  lsRequestId id;
  lsRequestId GetRequestId() override { return id; }
};

// NotificationMessage does not have |id|.
struct NotificationMessage : public BaseIpcMessage {};
