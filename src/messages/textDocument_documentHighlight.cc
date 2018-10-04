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
#include "symbol.h"

#include <algorithm>
using namespace ccls;

namespace {
MethodType kMethodType = "textDocument/documentHighlight";

struct lsDocumentHighlight {
  enum Kind { Text = 1, Read = 2, Write = 3 };

  lsRange range;
  int kind = 1;

  // ccls extension
  Role role = Role::None;

  bool operator<(const lsDocumentHighlight &o) const {
    return !(range == o.range) ? range < o.range : kind < o.kind;
  }
};
MAKE_REFLECT_STRUCT(lsDocumentHighlight, range, kind, role);

struct In_TextDocumentDocumentHighlight : public RequestInMessage {
  MethodType GetMethodType() const override { return kMethodType; }
  lsTextDocumentPositionParams params;
};
MAKE_REFLECT_STRUCT(In_TextDocumentDocumentHighlight, id, params);
REGISTER_IN_MESSAGE(In_TextDocumentDocumentHighlight);

struct Out_TextDocumentDocumentHighlight
    : public lsOutMessage<Out_TextDocumentDocumentHighlight> {
  lsRequestId id;
  std::vector<lsDocumentHighlight> result;
};
MAKE_REFLECT_STRUCT(Out_TextDocumentDocumentHighlight, jsonrpc, id, result);

struct Handler_TextDocumentDocumentHighlight
    : BaseMessageHandler<In_TextDocumentDocumentHighlight> {
  MethodType GetMethodType() const override { return kMethodType; }
  void Run(In_TextDocumentDocumentHighlight *request) override {
    int file_id;
    QueryFile *file;
    if (!FindFileOrFail(db, project, request->id,
                        request->params.textDocument.uri.GetPath(), &file,
                        &file_id))
      return;
    WorkingFile *working_file =
        working_files->GetFileByFilename(file->def->path);

    Out_TextDocumentDocumentHighlight out;
    out.id = request->id;

    std::vector<SymbolRef> syms = FindSymbolsAtLocation(
        working_file, file, request->params.position, true);
    for (auto [sym, refcnt] : file->symbol2refcnt) {
      if (refcnt <= 0)
        continue;
      Usr usr = sym.usr;
      SymbolKind kind = sym.kind;
      if (std::none_of(syms.begin(), syms.end(), [&](auto &sym1) {
            return usr == sym1.usr && kind == sym1.kind;
          }))
        continue;
      if (auto loc = GetLsLocation(db, working_files, sym, file_id)) {
        lsDocumentHighlight highlight;
        highlight.range = loc->range;
        if (sym.role & Role::Write)
          highlight.kind = lsDocumentHighlight::Write;
        else if (sym.role & Role::Read)
          highlight.kind = lsDocumentHighlight::Read;
        else
          highlight.kind = lsDocumentHighlight::Text;
        highlight.role = sym.role;
        out.result.push_back(highlight);
      }
    }
    std::sort(out.result.begin(), out.result.end());
    pipeline::WriteStdout(kMethodType, out);
  }
};
REGISTER_MESSAGE_HANDLER(Handler_TextDocumentDocumentHighlight);
} // namespace
