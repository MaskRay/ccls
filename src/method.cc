/* Copyright 2017-2018 ccls Authors

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

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
