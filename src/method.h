#pragma once

#include "serializer.h"
#include "utils.h"

#include <string>

using MethodType = std::string;
extern const char* kMethodType_Unknown;
extern const char* kMethodType_Exit;
extern const char* kMethodType_TextDocumentPublishDiagnostics;
extern const char* kMethodType_CqueryPublishInactiveRegions;
extern const char* kMethodType_CqueryPublishSemanticHighlighting;

using lsRequestId = std::variant<std::monostate, int64_t, std::string>;

struct InMessage {
  virtual ~InMessage();

  virtual MethodType GetMethodType() const = 0;
  virtual lsRequestId GetRequestId() const = 0;
};

struct RequestInMessage : public InMessage {
  // number or string, actually no null
  lsRequestId id;
  lsRequestId GetRequestId() const override;
};

// NotificationInMessage does not have |id|.
struct NotificationInMessage : public InMessage {
  lsRequestId GetRequestId() const override;
};
