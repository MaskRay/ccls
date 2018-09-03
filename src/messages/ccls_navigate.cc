// Copyright 2017-2018 ccls Authors
// SPDX-License-Identifier: Apache-2.0

#include "message_handler.h"
#include "pipeline.hh"
#include "query_utils.h"
using namespace ccls;

namespace {
MethodType kMethodType = "$ccls/navigate";

struct In_CclsNavigate : public RequestInMessage {
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
      Maybe<Range> parent;
      for (auto [sym, refcnt] : file->outline2refcnt)
        if (refcnt > 0 && sym.range.start <= pos && pos < sym.range.end &&
            (!parent || parent->start < sym.range.start))
          parent = sym.range;
      for (auto [sym, refcnt] : file->outline2refcnt)
        if (refcnt > 0 && pos < sym.range.start &&
            (!parent || sym.range.end <= parent->end) &&
            (!res || sym.range.start < res->start))
          res = sym.range;
      break;
    }
    case 'L':
      for (auto [sym, refcnt] : file->outline2refcnt)
        if (refcnt > 0 && sym.range.end <= pos &&
            (!res || (res->end == sym.range.end ? sym.range.start < res->start
                                                : res->end < sym.range.end)))
          res = sym.range;
      break;
    case 'R': {
      Maybe<Range> parent;
      for (auto [sym, refcnt] : file->outline2refcnt)
        if (refcnt > 0 && sym.range.start <= pos && pos < sym.range.end &&
            (!parent || parent->start < sym.range.start))
          parent = sym.range;
      if (parent && parent->start.line == pos.line && pos < parent->end) {
        pos = parent->end;
        if (pos.column)
          pos.column--;
      }
      for (auto [sym, refcnt] : file->outline2refcnt)
        if (refcnt > 0 && pos < sym.range.start &&
            (!res ||
             (sym.range.start == res->start ? res->end < sym.range.end
                                            : sym.range.start < res->start)))
          res = sym.range;
      break;
    }
    case 'U':
    default:
      for (auto [sym, refcnt] : file->outline2refcnt)
        if (refcnt > 0 && sym.range.start < pos && pos < sym.range.end &&
            (!res || res->start < sym.range.start))
          res = sym.range;
      break;
    }
    Out_LocationList out;
    out.id = request->id;
    if (res)
      if (auto ls_range = GetLsRange(wfile, *res)) {
        lsLocationEx &ls_loc = out.result.emplace_back();
        ls_loc.uri = params.textDocument.uri;
        ls_loc.range = *ls_range;
      }
    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_CclsNavigate);
} // namespace
