#pragma once

#include "serializer.h"
#include "utils.h"

#include <string>

using MethodType = const char*;
extern MethodType kMethodType_Unknown;
extern MethodType kMethodType_Exit;
extern MethodType kMethodType_TextDocumentPublishDiagnostics;
extern MethodType kMethodType_CclsPublishSkippedRanges;
extern MethodType kMethodType_CclsPublishSemanticHighlighting;

struct lsRequestId {
  // The client can send the request id as an int or a string. We should output
  // the same format we received.
  enum Type { kNone, kInt, kString };
  Type type = kNone;

  int value = -1;

  bool Valid() const { return type != kNone; }
};
void Reflect(Reader& visitor, lsRequestId& value);
void Reflect(Writer& visitor, lsRequestId& value);

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
    return lsRequestId();
  }
};
