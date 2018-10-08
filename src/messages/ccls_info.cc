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
#include "project.h"
#include "query_utils.h"
using namespace ccls;

MAKE_REFLECT_STRUCT(QueryFile::Def, path, args, language, skipped_ranges,
                    dependencies);

namespace {
MethodType cclsInfo = "$ccls/info", fileInfo = "$ccls/fileInfo";

struct In_cclsInfo : public RequestInMessage {
  MethodType GetMethodType() const override { return cclsInfo; }
};
MAKE_REFLECT_STRUCT(In_cclsInfo, id);
REGISTER_IN_MESSAGE(In_cclsInfo);

struct Out_cclsInfo : public lsOutMessage<Out_cclsInfo> {
  lsRequestId id;
  struct Result {
    struct DB {
      int files, funcs, types, vars;
    } db;
    struct Pipeline {
      int pendingIndexRequests;
    } pipeline;
    struct Project {
      int entries;
    } project;
  } result;
};
MAKE_REFLECT_STRUCT(Out_cclsInfo::Result::DB, files, funcs, types, vars);
MAKE_REFLECT_STRUCT(Out_cclsInfo::Result::Pipeline, pendingIndexRequests);
MAKE_REFLECT_STRUCT(Out_cclsInfo::Result::Project, entries);
MAKE_REFLECT_STRUCT(Out_cclsInfo::Result, db, pipeline, project);
MAKE_REFLECT_STRUCT(Out_cclsInfo, jsonrpc, id, result);

struct Handler_cclsInfo : BaseMessageHandler<In_cclsInfo> {
  MethodType GetMethodType() const override { return cclsInfo; }
  void Run(In_cclsInfo *request) override {
    Out_cclsInfo out;
    out.id = request->id;
    out.result.db.files = db->files.size();
    out.result.db.funcs = db->funcs.size();
    out.result.db.types = db->types.size();
    out.result.db.vars = db->vars.size();
    out.result.pipeline.pendingIndexRequests = pipeline::pending_index_requests;
    out.result.project.entries = 0;
    for (auto &[_, folder] : project->root2folder)
      out.result.project.entries += folder.entries.size();
    pipeline::WriteStdout(cclsInfo, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_cclsInfo);

struct In_cclsFileInfo : public RequestInMessage {
  MethodType GetMethodType() const override { return fileInfo; }
  struct Params {
    lsTextDocumentIdentifier textDocument;
  } params;
};
MAKE_REFLECT_STRUCT(In_cclsFileInfo::Params, textDocument);
MAKE_REFLECT_STRUCT(In_cclsFileInfo, id, params);
REGISTER_IN_MESSAGE(In_cclsFileInfo);

struct Out_cclsFileInfo : public lsOutMessage<Out_cclsFileInfo> {
  lsRequestId id;
  QueryFile::Def result;
};
MAKE_REFLECT_STRUCT(Out_cclsFileInfo, jsonrpc, id, result);

struct Handler_cclsFileInfo : BaseMessageHandler<In_cclsFileInfo> {
  MethodType GetMethodType() const override { return fileInfo; }
  void Run(In_cclsFileInfo *request) override {
    QueryFile *file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file))
      return;

    Out_cclsFileInfo out;
    out.id = request->id;
    // Expose some fields of |QueryFile::Def|.
    out.result.path = file->def->path;
    out.result.args = file->def->args;
    out.result.language = file->def->language;
    out.result.includes = file->def->includes;
    out.result.skipped_ranges = file->def->skipped_ranges;
    pipeline::WriteStdout(fileInfo, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_cclsFileInfo);
} // namespace
