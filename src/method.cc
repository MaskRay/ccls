// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "method.h"

MethodType kMethodType_Unknown = "$unknown";
MethodType kMethodType_Exit = "exit";
MethodType kMethodType_TextDocumentPublishDiagnostics =
    "textDocument/publishDiagnostics";
MethodType kMethodType_CclsPublishSkippedRanges = "$ccls/publishSkippedRanges";
MethodType kMethodType_CclsPublishSemanticHighlighting =
    "$ccls/publishSemanticHighlighting";

void Reflect(Reader &visitor, lsRequestId &value) {
  if (visitor.IsInt64()) {
    value.type = lsRequestId::kInt;
    value.value = int(visitor.GetInt64());
  } else if (visitor.IsInt()) {
    value.type = lsRequestId::kInt;
    value.value = visitor.GetInt();
  } else if (visitor.IsString()) {
    value.type = lsRequestId::kString;
    value.value = atoll(visitor.GetString());
  } else {
    value.type = lsRequestId::kNone;
    value.value = -1;
  }
}

void Reflect(Writer &visitor, lsRequestId &value) {
  switch (value.type) {
  case lsRequestId::kNone:
    visitor.Null();
    break;
  case lsRequestId::kInt:
    visitor.Int(value.value);
    break;
  case lsRequestId::kString:
    auto s = std::to_string(value.value);
    visitor.String(s.c_str(), s.length());
    break;
  }
}
