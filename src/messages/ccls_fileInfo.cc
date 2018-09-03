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

#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
using namespace ccls;

MAKE_REFLECT_STRUCT(QueryFile::Def, path, args, language, skipped_ranges,
                    dependencies);

namespace {
MethodType kMethodType = "$ccls/fileInfo";

struct lsDocumentSymbolParams {
  lsTextDocumentIdentifier textDocument;
};
MAKE_REFLECT_STRUCT(lsDocumentSymbolParams, textDocument);

struct In_CclsFileInfo : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsDocumentSymbolParams params;
};
MAKE_REFLECT_STRUCT(In_CclsFileInfo, id, params);
REGISTER_IN_MESSAGE(In_CclsFileInfo);

struct Out_CclsFileInfo : public lsOutMessage<Out_CclsFileInfo> {
  lsRequestId id;
  QueryFile::Def result;
};
MAKE_REFLECT_STRUCT(Out_CclsFileInfo, jsonrpc, id, result);

struct Handler_CclsFileInfo : BaseMessageHandler<In_CclsFileInfo> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_CclsFileInfo *request) override {
    QueryFile *file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file)) {
      return;
    }

    Out_CclsFileInfo out;
    out.id = request->id;
    // Expose some fields of |QueryFile::Def|.
    out.result.path = file->def->path;
    out.result.args = file->def->args;
    out.result.language = file->def->language;
    out.result.includes = file->def->includes;
    out.result.skipped_ranges = file->def->skipped_ranges;
    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CclsFileInfo);
} // namespace
