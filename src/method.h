#pragma once

#include "serializer.h"
#include "utils.h"

#include <string>

using MethodType = const char*;
extern MethodType kMethodType_Unknown;
extern MethodType kMethodType_Exit;
extern MethodType kMethodType_TextDocumentPublishDiagnostics;
extern MethodType kMethodType_CclsPublishInactiveRegions;
extern MethodType kMethodType_CclsPublishSemanticHighlighting;

using lsRequestId = std::variant<std::monostate, int64_t, std::string>;

struct InMessage {
  virtual ~InMessage() = default;

  virtual MethodType GetMethodType() const = 0;
  virtual lsRequestId GetRequestId() const = 0;
};

struct RequestInMessage : public InMessage {
  // number or string, actually no null
  lsRequestId id;
  lsRequestId GetRequestId() const override {
    return id;
  }
};

// NotificationInMessage does not have |id|.
struct NotificationInMessage : public InMessage {
  lsRequestId GetRequestId() const override {
    return std::monostate();
  }
};
