#pragma once

#include "serializer.h"
#include "utils.h"

#include <string>

using MethodType = std::string;
extern MethodType kMethodType_Unknown;
extern MethodType kMethodType_Exit;
extern MethodType kMethodType_TextDocumentPublishDiagnostics;
extern MethodType kMethodType_CqueryPublishInactiveRegions;
extern MethodType kMethodType_CqueryPublishSemanticHighlighting;

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
