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

struct In_cclsInfo : public RequestMessage {
  MethodType GetMethodType() const override { return cclsInfo; }
};
MAKE_REFLECT_STRUCT(In_cclsInfo, id);
REGISTER_IN_MESSAGE(In_cclsInfo);

struct Out_cclsInfo {
  struct DB {
    int files, funcs, types, vars;
  } db;
  struct Pipeline {
    int pendingIndexRequests;
  } pipeline;
  struct Project {
    int entries;
  } project;
};
MAKE_REFLECT_STRUCT(Out_cclsInfo::DB, files, funcs, types, vars);
MAKE_REFLECT_STRUCT(Out_cclsInfo::Pipeline, pendingIndexRequests);
MAKE_REFLECT_STRUCT(Out_cclsInfo::Project, entries);
MAKE_REFLECT_STRUCT(Out_cclsInfo, db, pipeline, project);

struct Handler_cclsInfo : BaseMessageHandler<In_cclsInfo> {
  MethodType GetMethodType() const override { return cclsInfo; }
  void Run(In_cclsInfo *request) override {
    Out_cclsInfo result;
    result.db.files = db->files.size();
    result.db.funcs = db->funcs.size();
    result.db.types = db->types.size();
    result.db.vars = db->vars.size();
    result.pipeline.pendingIndexRequests = pipeline::pending_index_requests;
    result.project.entries = 0;
    for (auto &[_, folder] : project->root2folder)
      result.project.entries += folder.entries.size();
    pipeline::Reply(request->id, result);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_cclsInfo);

struct In_cclsFileInfo : public RequestMessage {
  MethodType GetMethodType() const override { return fileInfo; }
  struct Params {
    lsTextDocumentIdentifier textDocument;
  } params;
};
MAKE_REFLECT_STRUCT(In_cclsFileInfo::Params, textDocument);
MAKE_REFLECT_STRUCT(In_cclsFileInfo, id, params);
REGISTER_IN_MESSAGE(In_cclsFileInfo);

struct Handler_cclsFileInfo : BaseMessageHandler<In_cclsFileInfo> {
  MethodType GetMethodType() const override { return fileInfo; }
  void Run(In_cclsFileInfo *request) override {
    QueryFile *file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file))
      return;

    QueryFile::Def result;
    // Expose some fields of |QueryFile::Def|.
    result.path = file->def->path;
    result.args = file->def->args;
    result.language = file->def->language;
    result.includes = file->def->includes;
    result.skipped_ranges = file->def->skipped_ranges;
    pipeline::Reply(request->id, result);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_cclsFileInfo);
} // namespace
