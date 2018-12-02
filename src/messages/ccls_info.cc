// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.hh"
#include "pipeline.hh"
#include "project.hh"
#include "query.hh"

namespace ccls {
MAKE_REFLECT_STRUCT(QueryFile::Def, path, args, language, skipped_ranges,
                    dependencies);

namespace {
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
} // namespace

void MessageHandler::ccls_info(EmptyParam &, ReplyOnce &reply) {
  Out_cclsInfo result;
  result.db.files = db->files.size();
  result.db.funcs = db->funcs.size();
  result.db.types = db->types.size();
  result.db.vars = db->vars.size();
  result.pipeline.pendingIndexRequests = pipeline::pending_index_requests;
  result.project.entries = 0;
  for (auto &[_, folder] : project->root2folder)
    result.project.entries += folder.entries.size();
  reply(result);
}

void MessageHandler::ccls_fileInfo(TextDocumentParam &param, ReplyOnce &reply) {
  QueryFile *file = FindFile(param.textDocument.uri.GetPath());
  if (!file)
    return;

  QueryFile::Def result;
  // Expose some fields of |QueryFile::Def|.
  result.path = file->def->path;
  result.args = file->def->args;
  result.language = file->def->language;
  result.includes = file->def->includes;
  result.skipped_ranges = file->def->skipped_ranges;
  reply(result);
}
} // namespace ccls
