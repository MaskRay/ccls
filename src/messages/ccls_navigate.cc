// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
using namespace ccls;

namespace {
MethodType kMethodType = "$ccls/navigate";

struct In_CclsNavigate : public RequestMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  struct Params {
    lsTextDocumentIdentifier textDocument;
    lsPosition position;
    std::string direction;
  } params;
};
MAKE_REFLECT_STRUCT(In_CclsNavigate::Params, textDocument, position, direction);
MAKE_REFLECT_STRUCT(In_CclsNavigate, id, params);
REGISTER_IN_MESSAGE(In_CclsNavigate);

Maybe<Range> FindParent(QueryFile *file, Position pos) {
  Maybe<Range> parent;
  for (auto [sym, refcnt] : file->symbol2refcnt)
    if (refcnt > 0 && sym.extent.Valid() && sym.extent.start <= pos &&
        pos < sym.extent.end &&
        (!parent || (parent->start == sym.extent.start
                         ? parent->end < sym.extent.end
                         : parent->start < sym.extent.start)))
      parent = sym.extent;
  return parent;
}

struct Handler_CclsNavigate : BaseMessageHandler<In_CclsNavigate> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_CclsNavigate *request) override {
    auto &params = request->params;
    QueryFile *file;
    if (!FindFileOrFail(db, project, request->id,
                        params.textDocument.uri.GetPath(), &file))
      return;

    WorkingFile *wfile =
        working_files->GetFileByFilename(file->def->path);
    lsPosition ls_pos = request->params.position;
    if (wfile && wfile->index_lines.size())
      if (auto line = wfile->GetIndexPosFromBufferPos(
          ls_pos.line, &ls_pos.character, false))
        ls_pos.line = *line;
    Position pos{(int16_t)ls_pos.line, (int16_t)ls_pos.character};

    Maybe<Range> res;
    switch (params.direction[0]) {
    case 'D': {
      Maybe<Range> parent = FindParent(file, pos);
      for (auto [sym, refcnt] : file->symbol2refcnt)
        if (refcnt > 0 && pos < sym.extent.start &&
            (!parent || sym.extent.end <= parent->end) &&
            (!res || sym.extent.start < res->start))
          res = sym.extent;
      break;
    }
    case 'L':
      for (auto [sym, refcnt] : file->symbol2refcnt)
        if (refcnt > 0 && sym.extent.Valid() && sym.extent.end <= pos &&
            (!res || (res->end == sym.extent.end ? sym.extent.start < res->start
                                                : res->end < sym.extent.end)))
          res = sym.extent;
      break;
    case 'R': {
      Maybe<Range> parent = FindParent(file, pos);
      if (parent && parent->start.line == pos.line && pos < parent->end) {
        pos = parent->end;
        if (pos.column)
          pos.column--;
      }
      for (auto [sym, refcnt] : file->symbol2refcnt)
        if (refcnt > 0 && sym.extent.Valid() && pos < sym.extent.start &&
            (!res ||
             (sym.extent.start == res->start ? res->end < sym.extent.end
                                            : sym.extent.start < res->start)))
          res = sym.extent;
      break;
    }
    case 'U':
    default:
      for (auto [sym, refcnt] : file->symbol2refcnt)
        if (refcnt > 0 && sym.extent.Valid() && sym.extent.start < pos &&
            pos < sym.extent.end && (!res || res->start < sym.extent.start))
          res = sym.extent;
      break;
    }
    std::vector<lsLocation> result;
    if (res)
      if (auto ls_range = GetLsRange(wfile, *res)) {
        lsLocation &ls_loc = result.emplace_back();
        ls_loc.uri = params.textDocument.uri;
        ls_loc.range = *ls_range;
      }
    pipeline::Reply(request->id, result);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CclsNavigate);
} // namespace
